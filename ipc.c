/* ipc.c - IPC over TCP.
 *
 * Wire format (little-endian only for v1):
 *
 *   off 0  arch     u8   1 = little-endian; anything else -> close conn
 *   off 1  msgtype  u8   0=async, 1=sync request, 2=sync response,
 *                        3=sync error
 *   off 2  version  u8   1; future-proofing for protocol changes
 *   off 3  reserved u8   must be 0
 *   off 4  msglen   u32  total message length INCLUDING this 8-byte header
 *   off 8  payload  ...  bd_-serialized K value; (msglen - 8) of them
 *
 * SYNC_ERR payloads are bd_-serialized char vectors (the error string),
 * so receivers can uniformly db_ everything and only special-case the
 * msgtype, not the encoding.
 *
 * Connections are persistent (q-style): the fd returned to k-space is the
 * real socket fd; many messages can flow over it; 3:w closes it.
 *
 * Portability:
 *   The same body of code below compiles on POSIX and Win32. Platform
 *   differences are isolated to the small shim block right under the
 *   includes (sock_*, ipc_init, ipc_shutdown). On Windows the server
 *   half is reachable, but until repl.c grows a Win32-aware poll loop
 *   nothing will service incoming connections during the prompt; client
 *   3:/4: calls work because they drive the shim's poll directly.
 */

#include "ipc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "k.core/types.h"
#include "k.core/x.h"
#include "k.core/rand.h"
#include "k.h"
#include "scope.h"
#include "fn.h"
#include "p.h"
#include "b.h"
#include "tmr.h"

/* ===== platform shim ===================================================== */

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #ifdef _MSC_VER
    #pragma comment(lib, "Ws2_32.lib")
    typedef int ssize_t;          /* recv/send return int; not in MSVC headers */
  #endif
  typedef SOCKET sock_t;
  #define INVALID_SOCK              INVALID_SOCKET
  #define sock_close(fd)            closesocket(fd)
  #define sock_recv(fd, buf, n)     recv((fd), (char*)(buf), (int)(n), 0)
  #define sock_send(fd, buf, n)     send((fd), (const char*)(buf), (int)(n), 0)
  #define sock_lasterr()            WSAGetLastError()
  #define sock_would_block(e)       ((e) == WSAEWOULDBLOCK)
  #define sock_intr(e)              (0)              /* not raised on winsock */
  #define sock_poll(p, n, t)        WSAPoll((p), (n), (t))
  static int sock_setnb(sock_t fd) {
    u_long nb = 1;
    return ioctlsocket(fd, FIONBIO, &nb) == 0 ? 0 : -1;
  }
  /* FormatMessage into a thread-local buffer; trim trailing CRLF. */
  static const char *sock_errstr(int err) {
    static __declspec(thread) char buf[256];
    DWORD n = FormatMessageA(
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, (DWORD)err, 0, buf, sizeof buf, NULL);
    while(n && (buf[n-1] == '\n' || buf[n-1] == '\r' || buf[n-1] == ' '))
      buf[--n] = 0;
    if(n == 0) snprintf(buf, sizeof buf, "winsock error %d", err);
    return buf;
  }
#else
  #include <unistd.h>
  #include <fcntl.h>
  #include <poll.h>
  #include <sys/socket.h>
  #include <sys/types.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>  /* TCP_NODELAY */
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <errno.h>
  #include <signal.h>     /* SIGCHLD reaping in fork mode */
  #include <sys/wait.h>   /* waitpid() in our reaper */
  #include <sys/resource.h> /* getrlimit() in close_inherited_fds() */
  #include <dirent.h>     /* opendir() in close_inherited_fds() */
  #include <ctype.h>      /* isdigit() in close_inherited_fds() */
  typedef int sock_t;
  #define INVALID_SOCK              (-1)
  #define sock_close(fd)            close(fd)
  #define sock_recv(fd, buf, n)     recv((fd), (buf), (n), 0)
  #define sock_send(fd, buf, n)     send((fd), (buf), (n), 0)
  #define sock_lasterr()            errno
  #define sock_would_block(e)       ((e) == EAGAIN || (e) == EWOULDBLOCK)
  #define sock_intr(e)              ((e) == EINTR)
  #define sock_poll(p, n, t)        poll((p), (n), (t))
  #define sock_errstr(e)            strerror(e)
  static int sock_setnb(int fd) {
    int fl = fcntl(fd, F_GETFL, 0);
    if(fl < 0) return -1;
    return fcntl(fd, F_SETFL, fl | O_NONBLOCK) < 0 ? -1 : 0;
  }
#endif

/* Disable Nagle on a connected data socket. With Nagle + delayed-ACK on the
 * other end, small request/response pairs stall by ~40ms per round trip
 * (observed: 100 sync callbacks from a fork child over a single dedup'd
 * conn ran at ~13/s instead of thousands/s). IPC traffic is interactive
 * and frame-oriented, so Nagle buys nothing and hurts throughput badly.
 * Fire-and-forget: a setsockopt failure is not fatal, the conn still
 * works, just slower. Only call on DATA sockets (accepted/connected),
 * never on listeners. */
static void sock_nodelay(sock_t fd) {
  int one = 1;
  (void)setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                   (const char*)&one, sizeof one);
}

int ipc_init(void) {
#ifdef _WIN32
  WSADATA wsa;
  return WSAStartup(MAKEWORD(2,2), &wsa) == 0 ? 0 : -1;
#else
  return 0;
#endif
}

/* ipc_shutdown is defined near the bottom of this file (after the static
 * conn table, send_scratch, and close_conn_at it needs to touch). */

/* Connection-table sizing (used by both the multiplexer and the impl). */
#define MAX_CONNS    256
#define POLL_BATCH   (1 + MAX_CONNS)   /* listen + all conns */

/* Hoisted up so the win32 multiplexer can distinguish the listen sockets
 * (which want FD_ACCEPT) from accepted/client conns (which want FD_READ
 * + FD_CLOSE). The full impl that mutates these lives further down.
 *
 * Two independent slots so `\m i N` and `\m f M` can both be active at
 * once (matching k's behavior). Each slot is rebound by `\m i/f PORT`
 * and torn down by `\m i/f 0`. */
static int listen_iter_fd   = -1;
static int listen_iter_port =  0;
static int listen_fork_fd   = -1;
static int listen_fork_port =  0;

#ifndef _WIN32
/* SIGCHLD handler used in fork mode. Reaps any number of exited children
 * non-blockingly; safe to leave installed even if no children are pending.
 * Preserves errno so it won't poison whatever syscall it interrupted. */
static void sigchld_reap(int sig) {
  (void)sig;
  int saved = errno;
  while(waitpid(-1, NULL, WNOHANG) > 0) {}
  errno = saved;
}

/* Close every fd we inherited from the parent except 0/1/2 and `keep`.
 *
 * Why: fork can fire mid-script-load (the load() loop in repl.c holds an
 * open FILE* on the script, and pgreduce can call w 4:msg which parks
 * ipc_send_sync in poll, which calls handle_accept). The child inherits
 * that FILE*'s fd; if the child ever read/wrote it, the shared kernel
 * offset would corrupt the parent's parse. We don't do that today, but
 * also: leaving the fd open is a leak and a footgun for future code.
 *
 * Also covers any other live FILE* from io.c (`0:`, `2:`, etc.) that
 * happens to be open at fork time.
 *
 * Strategy: prefer /proc/self/fd (Linux) or /dev/fd (macOS) for an exact
 * list of currently open fds. Fall back to a getrlimit-bounded loop. */
static void close_inherited_fds(int keep) {
  const char *paths[] = { "/proc/self/fd", "/dev/fd", NULL };
  for(int p = 0; paths[p]; p++) {
    DIR *d = opendir(paths[p]);
    if(!d) continue;
    int dfd = dirfd(d);
    /* Snapshot fds first so we don't perturb the iteration by closing. */
    int  buf[256];
    int  bn = 0;
    struct dirent *e;
    while((e = readdir(d))) {
      if(!isdigit((unsigned char)e->d_name[0])) continue;
      int fd = atoi(e->d_name);
      if(fd <= 2 || fd == keep || fd == dfd) continue;
      if(bn < (int)(sizeof buf / sizeof buf[0])) buf[bn++] = fd;
    }
    closedir(d);
    for(int i = 0; i < bn; i++) (void)close(buf[i]);
    return;
  }
  /* Fallback: brute-force range. */
  struct rlimit rl;
  int max = 1024;
  if(getrlimit(RLIMIT_NOFILE, &rl) == 0 && rl.rlim_cur != RLIM_INFINITY)
    max = (int)rl.rlim_cur;
  for(int fd = 3; fd < max; fd++)
    if(fd != keep) (void)close(fd);
}
#endif

/* ===== Windows stdin multiplexer ========================================
 *
 * Goal: let the repl block on stdin AND on IPC sockets at the same time,
 * the way poll() does on POSIX. WSAPoll only works on sockets - and any
 * loopback bind+listen "self-pipe" pair triggers the Windows Firewall
 * "allow public/private network access" prompt at startup, which is
 * unfriendly when the user hasn't asked to listen on anything yet.
 *
 * So we use a pure Win32 setup with NO sockets until the user explicitly
 * runs \m i PORT or passes -i PORT:
 *
 *   reader thread          repl_getc / WaitForMultipleObjects
 *      |                       |
 *   ReadFile(stdin)            wait set: [stdin_event,
 *      |                                  WSAEvent(listen_iter_fd),
 *   ring_push(bytes)                      WSAEvent(conn_fd)...]
 *      |                       |
 *   SetEvent(stdin_event)      stdin_event fires -> drain ring;
 *                              socket events -> WSAEnumNetworkEvents,
 *                              translate to revents, ipc_handle_pollfd.
 *
 * stdin_event is a manual-reset Win32 Event:
 *   - reader thread sets it whenever bytes are pushed or EOF is hit;
 *   - ring_pop resets it once it's drained the ring AND eof isn't set.
 * The IPC socket events are created/torn down per wait iteration with
 * WSAEventSelect, which is slightly wasteful but keeps the code small
 * and avoids per-conn lifecycle bookkeeping. */

#ifdef _WIN32

#define STDIN_WAIT_BUDGET (MAXIMUM_WAIT_OBJECTS - 1)  /* leave slot for stdin */

static HANDLE           stdin_event = NULL;       /* manual-reset */
static HANDLE           reader_th   = NULL;
static CRITICAL_SECTION ring_cs;
static unsigned char   *ring        = NULL;
static size_t           ring_cap    = 0;
static size_t           ring_head   = 0;          /* next byte to read */
static size_t           ring_tail   = 0;          /* one past last byte */
static int              stdin_eof   = 0;

/* Recompute stdin_event state from ring contents. Caller holds ring_cs. */
static void update_stdin_event_locked(void) {
  if(ring_head < ring_tail || stdin_eof) SetEvent(stdin_event);
  else                                   ResetEvent(stdin_event);
}

static void ring_push(const unsigned char *p, size_t n) {
  EnterCriticalSection(&ring_cs);
  /* compact if there's slack at the front */
  if(ring_head > 0) {
    memmove(ring, ring + ring_head, ring_tail - ring_head);
    ring_tail -= ring_head;
    ring_head  = 0;
  }
  if(ring_tail + n > ring_cap) {
    size_t newcap = ring_cap ? ring_cap * 2 : 4096;
    while(newcap < ring_tail + n) newcap *= 2;
    ring = xrealloc(ring, newcap);
    ring_cap = newcap;
  }
  memcpy(ring + ring_tail, p, n);
  ring_tail += n;
  update_stdin_event_locked();
  LeaveCriticalSection(&ring_cs);
}

/* 1=byte returned, 0=empty (caller must wait), -1=eof and empty */
static int ring_pop(unsigned char *out) {
  int rc;
  EnterCriticalSection(&ring_cs);
  if(ring_head < ring_tail) { *out = ring[ring_head++]; rc = 1; }
  else if(stdin_eof)        { rc = -1; }
  else                      { rc = 0; }
  update_stdin_event_locked();
  LeaveCriticalSection(&ring_cs);
  return rc;
}

static DWORD WINAPI reader_main(LPVOID arg) {
  (void)arg;
  HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
  unsigned char buf[4096];
  for(;;) {
    DWORD nread = 0;
    BOOL  ok    = ReadFile(hin, buf, sizeof buf, &nread, NULL);
    if(!ok || nread == 0) {
      EnterCriticalSection(&ring_cs);
      stdin_eof = 1;
      update_stdin_event_locked();
      LeaveCriticalSection(&ring_cs);
      return 0;
    }
    ring_push(buf, (size_t)nread);
  }
}

int ipc_stdin_init(void) {
  InitializeCriticalSection(&ring_cs);
  stdin_event = CreateEvent(NULL, TRUE /*manual reset*/, FALSE, NULL);
  if(!stdin_event) return -1;
  reader_th = CreateThread(NULL, 0, reader_main, NULL, 0, NULL);
  return reader_th ? 0 : -1;
}

/* Snapshot the currently pollable IPC sockets and create a WSAEvent for
 * each. socks[i] is the SOCKET; events[i] is the parallel Win32 event
 * handle (which is what WaitForMultipleObjects waits on). Capped at
 * STDIN_WAIT_BUDGET because WaitForMultipleObjects tops out at 64 handles
 * total; if a user ever has >63 active conns at once, the older ones will
 * still drain but with a slight latency. */
static int build_socket_events(WSAEVENT *events, sock_t *socks) {
  struct pollfd tmp[POLL_BATCH];
  int n = ipc_extra_pollfds(tmp, POLL_BATCH);
  if(n > STDIN_WAIT_BUDGET) n = STDIN_WAIT_BUDGET;
  for(int i = 0; i < n; i++) {
    events[i] = WSACreateEvent();
    socks[i]  = (sock_t)tmp[i].fd;
    long mask = FD_READ | FD_CLOSE;
    if((int)socks[i] == listen_iter_fd ||
       (int)socks[i] == listen_fork_fd) mask |= FD_ACCEPT;
    /* WSAEventSelect implicitly sets the socket non-blocking, which is
     * already true for ours. It does NOT auto-restore blocking on detach,
     * so the un-WSAEventSelect at teardown leaves them as-is. */
    WSAEventSelect(socks[i], events[i], mask);
  }
  return n;
}

static void teardown_socket_events(WSAEVENT *events, sock_t *socks, int n) {
  for(int i = 0; i < n; i++) {
    WSAEventSelect(socks[i], NULL, 0);
    WSACloseEvent(events[i]);
  }
}

int ipc_stdin_getc(void) {
  unsigned char c;
  for(;;) {
    int r = ring_pop(&c);
    if(r ==  1) return (int)c;
    if(r == -1) return -1;

    HANDLE  handles[MAXIMUM_WAIT_OBJECTS];
    sock_t  socks  [STDIN_WAIT_BUDGET];
    handles[0]  = stdin_event;
    int n_socks = build_socket_events((WSAEVENT*)(handles + 1), socks);
    int    tms  = tmr_timeout_ms();
    DWORD  wms  = (tms < 0) ? INFINITE : (DWORD)tms;
    DWORD wr = WaitForMultipleObjects((DWORD)(1 + n_socks), handles,
                                       FALSE, wms);

    if(wr == WAIT_TIMEOUT) {
      teardown_socket_events((WSAEVENT*)(handles + 1), socks, n_socks);
      tmr_maybe_fire();
      continue;
    }

    if(wr >= WAIT_OBJECT_0 && wr < WAIT_OBJECT_0 + (DWORD)(1 + n_socks)) {
      DWORD idx = wr - WAIT_OBJECT_0;
      if(idx == 0) {
        /* stdin event - ring_pop will service it on the next iteration */
      } else {
        int si = (int)(idx - 1);
        WSANETWORKEVENTS ev;
        if(WSAEnumNetworkEvents(socks[si], handles[idx], &ev) == 0) {
          short rev = 0;
          if(ev.lNetworkEvents & (FD_READ|FD_ACCEPT)) rev |= POLLIN;
          if(ev.lNetworkEvents & FD_CLOSE)            rev |= POLLHUP;
          if(rev) ipc_handle_pollfd((int)socks[si], rev);
        }
      }
      teardown_socket_events((WSAEVENT*)(handles + 1), socks, n_socks);
      tmr_maybe_fire();
    } else {
      teardown_socket_events((WSAEVENT*)(handles + 1), socks, n_socks);
      return -1;
    }
  }
}

#else

int ipc_stdin_init(void) { return 0; }
/* ipc_stdin_getc is unused on POSIX (repl uses poll_getc directly). */

#endif

/* ===== implementation (single body, both platforms) ====================== */


#define IPC_HDR_SIZE   8
#define IPC_VERSION    1
#define IPC_ARCH_LE    1
/* Hard cap per message (header + payload). The real ceiling is set by:
 *   - our wire framing: body_need is i32; >2^31 wraps negative
 *   - K's public count APIs (tn/tnv/kresize) take i32, so a single K
 *     vector tops out at ~2.1G elements anyway
 * Senders that exceed this raise 'length; receivers that see a larger
 * msglen drop the conn (only reachable from a non-conforming peer at
 * this point). */
#define IPC_MAX_MSG  ((u32)0x7FFFFFFF)         /* INT32_MAX */

#define MSG_ASYNC     0
#define MSG_SYNC_REQ  1
#define MSG_SYNC_RSP  2
#define MSG_SYNC_ERR  3

/* Per-connection receive state.
 *
 * The state machine runs in two phases:
 *   1. accumulating header (hdr_have < IPC_HDR_SIZE)
 *   2. accumulating body   (body != NULL && body_have < body_need)
 * On completion we hand the message to deliver_message() and reset
 * for the next message.
 *
 * is_client is set on conns we opened via ipc_open(); cleared on conns
 * we accept()ed. waiting_sync/sync_have/sync_response/sync_is_err form
 * the rendezvous slot for an in-progress ipc_send_sync(): when a
 * SYNC_RSP/SYNC_ERR arrives on a conn whose waiting_sync is set, the
 * dispatcher drops the result into the slot rather than logging it. */
typedef struct {
  int   fd;
  /* receive */
  u8    hdr[IPC_HDR_SIZE];
  i32   hdr_have;
  u8   *body;          /* xmalloc'd; persists across messages until close */
  i32   body_cap;      /* current backing allocation; >= body_need on a parsed header */
  i32   body_have;
  i32   body_need;     /* bytes of bd payload expected (msglen - 8) */
  u8    msgtype;       /* parsed from header; valid once body!=NULL */
  /* role / sync rendezvous */
  u8    is_client;     /* 1 if we opened it (vs accepted) */
  u8    waiting_sync;  /* 1 while ipc_send_sync is parked on this fd */
  u8    sync_have;     /* 1 once sync_response is filled */
  u8    sync_is_err;   /* 1 if it arrived as MSG_SYNC_ERR */
  u8    in_dispatch;   /* 1 while a handler (.m.s/.m.g/.m.c) runs for this
                          conn. nested sync waits exclude this fd from
                          their poll set so more data on the same conn
                          queues in the kernel instead of re-entering
                          dispatch. */
  K     sync_response; /* deserialized payload; valid iff sync_have */
  /* client-only: dedup key. matches q's behavior of returning the same
   * handle for repeated 3:(host;port) on the same target rather than
   * burning a new socket each time. */
  char  host[256];     /* normalized host string, valid iff is_client */
  i32   port;          /* tcp port,                valid iff is_client */
} ipc_conn;

/* listen_{iter,fork}_{fd,port} are defined above the win32 multiplexer block. */
static ipc_conn conns[MAX_CONNS];
static int      conn_count  =  0;

/* Report the bound port for one slot, 0 if inactive. The two slots
 * are independent; callers that want to summarize both query each. */
int ipc_listen_port_for(int mode) {
  if(mode == IPC_MODE_FORK) return listen_fork_fd >= 0 ? listen_fork_port : 0;
  return                            listen_iter_fd >= 0 ? listen_iter_port : 0;
}

/* Fuzz builds short-circuit every IPC entry point that touches the
 * outside world (sockets, DNS, fork, blocking reads). Rationale:
 *   - bind/listen on random ports collides across forks and leaks fds
 *   - getaddrinfo on arbitrary hostnames can stall on DNS
 *   - connect to localhost may hit unrelated services (ssh, postgres, ...)
 *   - ipc_send_sync would block indefinitely waiting for a reply that
 *     never arrives, hanging the fuzzer
 * ipc_init_ns is intentionally NOT stubbed: it only mutates K state
 * (.m.{s,g,c}) and gives the fuzzer something to exercise. */

/* ---- connection table helpers ---- */

static int find_conn(int fd) {
  for(int i = 0; i < conn_count; i++) if(conns[i].fd == fd) return i;
  return -1;
}

/* Reset for the next message. Keeps c->body / c->body_cap so subsequent
 * messages on the same conn don't pay the alloc + page-fault cost again. */
static void reset_recv(ipc_conn *c) {
  c->hdr_have  = 0;
  c->body_have = 0;
  c->body_need = 0;
  c->msgtype   = 0;
}

/* Forward decl for the close callback. */
static void fire_close_handler(int fd);

/* Forward decl: wrap a strerror as a kerror with a leading prefix
 * (e.g. "bind"). Defined later, near the client open/close path. */
static K cerror(const char *prefix, int err);

static void close_conn_at(int idx) {
  ipc_conn *c = &conns[idx];
  int fd = c->fd;
  /* If a sync caller is parked on this fd, hand them an error so they
   * don't block forever. We set sync_have here; ipc_send_sync notices
   * the conn vanished and synthesizes a 'conn' error. */
  u8 was_waiter = c->waiting_sync && !c->sync_have;
  sock_close(fd);
  reset_recv(c);
  /* reset_recv keeps the body buffer alive across messages; on close
   * we must free it ourselves before the slot is overwritten below. */
  if(c->body) { xfree(c->body); c->body = NULL; c->body_cap = 0; }
  if(c->sync_have) {
    /* leftover response we never delivered */
    if(c->sync_response && !E(c->sync_response)) _k(c->sync_response);
    c->sync_have = 0;
    c->sync_response = 0;
  }
  conns[idx] = conns[--conn_count];
  /* Fire .m.c for every closed handle, regardless of whether we accepted
   * it (server side) or opened it (client side). */
  fire_close_handler(fd);
  (void)was_waiter; /* sync caller will detect missing conn on next iter */
}

/* ---- listening / accepting ---- */

K ipc_listen(int port, int mode) {
#ifdef FUZZING
  /* fuzz: pretend success without binding; \m / \m i N stay quiet */
  (void)port; (void)mode; return 0;
#else
#ifdef _WIN32
  if(mode == IPC_MODE_FORK) return kerror("fork: not supported on windows");
#endif
  if(mode != IPC_MODE_ITER && mode != IPC_MODE_FORK) return KERR_DOMAIN;

  /* Each mode owns an independent listener slot; rebinding or stopping
   * (port==0) only affects the slot for the requested mode. */
  int *slot_fd   = (mode == IPC_MODE_FORK) ? &listen_fork_fd   : &listen_iter_fd;
  int *slot_port = (mode == IPC_MODE_FORK) ? &listen_fork_port : &listen_iter_port;

  /* port==0 just stops this slot. */
  if(port == 0) {
    if(*slot_fd >= 0) { sock_close(*slot_fd); *slot_fd = -1; *slot_port = 0; }
    return 0;
  }
  if(port < 0 || port > 65535) return KERR_DOMAIN;

  /* Reject rebinding to the port this slot is already bound to — it's
   * redundant, and silently re-creating a socket hides bugs. User must
   * `\m i 0` / `\m f 0` first to explicitly stop, then rebind. */
  if(*slot_fd >= 0 && *slot_port == port) return kerror("bind: Address already in use");

  /* Do NOT close the old slot yet. We'll open the new socket and bind it
   * first; only tear the old one down on success. That way a failed bind
   * (e.g. the requested port is owned by the other slot, or by a process
   * outside gk) leaves the original listener intact. */

  sock_t fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd == INVALID_SOCK) return cerror("socket", sock_lasterr());
  int one = 1;
  (void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                   (const char*)&one, sizeof one);

  struct sockaddr_in sa;
  memset(&sa, 0, sizeof sa);
  sa.sin_family      = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port        = htons((unsigned short)port);

  if(bind(fd, (struct sockaddr*)&sa, sizeof sa) < 0) {
    K e = cerror("bind", sock_lasterr());
    sock_close(fd);
    return e;
  }
  if(listen(fd, 16) < 0) {
    K e = cerror("listen", sock_lasterr());
    sock_close(fd);
    return e;
  }
#ifndef _WIN32
  /* In fork mode, install a real SIGCHLD handler that reaps exited
   * children non-blockingly. We deliberately DON'T use SIG_IGN here:
   * POSIX says with SIG_IGN, waitpid() blocks until ALL children exit
   * and then fails with ECHILD, which would break `4: and `8: (the
   * shell-out verbs in io.c that fork+waitpid for a specific pid).
   * Idempotent: re-installing the same handler is a no-op. */
  if(mode == IPC_MODE_FORK) {
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = sigchld_reap;
    sa.sa_flags   = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);
  }
#endif
  /* New socket is bound+listening. Now safe to retire the old slot (if
   * any) and publish the new one. */
  if(*slot_fd >= 0) { sock_close(*slot_fd); *slot_fd = -1; *slot_port = 0; }
  *slot_fd   = (int)fd;
  *slot_port = port;
  return 0;
#endif
}

int ipc_extra_pollfds(struct pollfd *fds, int max) {
#ifdef FUZZING
  /* fuzz: no listener, no conns -> nothing to poll */
  (void)fds; (void)max; return 0;
#else
  int n = 0;
  if(listen_iter_fd >= 0 && n < max) {
    fds[n].fd = listen_iter_fd; fds[n].events = POLLIN; fds[n].revents = 0; n++;
  }
  if(listen_fork_fd >= 0 && n < max) {
    fds[n].fd = listen_fork_fd; fds[n].events = POLLIN; fds[n].revents = 0; n++;
  }
  for(int i = 0; i < conn_count && n < max; i++) {
    /* Skip conns whose handler is currently running - this call is
     * reached from inside a sync wait nested in that handler. Pulling
     * more data off the conn now would re-enter dispatch_async on the
     * same fd, and the inner handler's ipc_send_sync (if it does one)
     * would collide with the outer pending sync handle via dedup and
     * fail as "sync re-entry on same handle". Leaving the fd out of the
     * poll set lets new messages queue in the kernel buffer; the outer
     * loop picks them up after the handler returns. */
    if(conns[i].in_dispatch) continue;
    fds[n].fd = conns[i].fd; fds[n].events = POLLIN; fds[n].revents = 0; n++;
  }
  return n;
#endif
}

#ifndef _WIN32
/* Run a child server process: dispatch messages on its single accepted
 * connection until the peer disconnects, then _exit. The child inherits
 * all K state from the parent via copy-on-write, so .m.{s,g,c} and any
 * user-defined globals are already in place; we just need to drain the
 * one fd we kept and let the existing recv_step / deliver_message machine
 * do its thing.
 *
 * Uses _exit (not exit) so atexit handlers don't interleave stdio with
 * the parent. The shared stdout/stderr is intentional: it matches q's
 * forking server behavior. */
static void ipc_child_loop(int cfd) {
  for(;;) {
    if(find_conn(cfd) < 0) break;            /* close_conn_at fired .m.c */
    struct pollfd pfd = { cfd, POLLIN, 0 };
    int pr = sock_poll(&pfd, 1, -1);
    if(pr < 0) {
      if(sock_intr(sock_lasterr())) continue;
      break;
    }
    if(pfd.revents) ipc_handle_pollfd(cfd, pfd.revents);
  }
  _exit(0);
}
#endif

static void handle_accept(int lfd, int lmode) {
  struct sockaddr_in sa;
  socklen_t          sl = sizeof sa;
  sock_t cfd = accept(lfd, (struct sockaddr*)&sa, &sl);
  if(cfd == INVALID_SOCK) return; /* transient: would-block or kernel hiccup */
  if(conn_count >= MAX_CONNS) { sock_close(cfd); return; }

#ifndef _WIN32
  if(lmode == IPC_MODE_FORK) {
    /* Flush both stdio streams BEFORE forking. Otherwise any data buffered
     * in the parent's FILE* gets duplicated into the child's copy and emits
     * twice the next time someone flushes. stdout is _IONBF in main.c so
     * usually empty, but stderr is line-buffered by default and a partial
     * line would re-emit. Cheap insurance. */
    fflush(stdout);
    fflush(stderr);
    pid_t pid = fork();
    if(pid < 0) {                            /* fork failed: drop the conn */
      sock_close(cfd);
      return;
    }
    if(pid > 0) {                            /* parent: hand off and resume */
      sock_close(cfd);
      return;
    }
    /* child: tear down inherited parent state we don't own. Close BOTH
     * listener slots: the child serves a single accepted conn and has no
     * business accepting new ones on either port. */
    if(listen_iter_fd >= 0) { sock_close(listen_iter_fd); listen_iter_fd = -1; listen_iter_port = 0; }
    if(listen_fork_fd >= 0) { sock_close(listen_fork_fd); listen_fork_fd = -1; listen_fork_port = 0; }
    for(int i = 0; i < conn_count; i++) sock_close(conns[i].fd);
    conn_count = 0;
    /* drop any inherited timer schedule. We don't want a child
     * staying alive past its work just because the parent had a
     * recurring timer set. */
    tmr_fork_clear();
    /* Drop any other fds the parent had open (script files mid-\l, any
     * io.c FILE* still live at fork time, etc). Keeps the new cfd. */
    close_inherited_fds((int)cfd);
    scope_refresh_pid();                     /* .z.P -> child pid */
    /* Reseed the xorshift RNG so siblings don't generate identical
     * sequences. Mix in pid + wall-clock so two children forked in the
     * same second still diverge (different pids). */
    rand_reseed((unsigned long)getpid() ^ ((unsigned long)time(NULL) << 16));
    /* register the one fd we DO own and dispatch on it. */
    (void)sock_setnb(cfd);
    sock_nodelay(cfd);
    ipc_conn *c = &conns[conn_count++];
    memset(c, 0, sizeof *c);
    c->fd = (int)cfd;
    c->is_client = 0;
    ipc_child_loop((int)cfd);                /* never returns */
  }
#endif

  /* Non-blocking so partial reads in recv_step return would-block cleanly. */
  (void)sock_setnb(cfd);
  sock_nodelay(cfd);
  ipc_conn *c = &conns[conn_count++];
  memset(c, 0, sizeof *c);
  c->fd = (int)cfd;
  c->is_client = 0;
}

/* ---- frame parsing ---- */

/* Validate a fully-received 8-byte header and populate body_need / msgtype.
 * Returns 0 on ok, -1 on protocol error (caller closes the conn). */
static int parse_header(ipc_conn *c) {
  u8  arch     = c->hdr[0];
  u8  msgtype  = c->hdr[1];
  u8  version  = c->hdr[2];
  u8  reserved = c->hdr[3];
  u32 msglen;
  memcpy(&msglen, c->hdr + 4, 4);   /* little-endian on supported archs */

  /* Any framing problem -> drop the connection. The caller (recv_step)
   * closes the fd, which in turn signals any sync waiter via the existing
   * close-with-waiter path; async senders just observe the closed peer. */
  if(arch != IPC_ARCH_LE)        return -1;
  if(version != IPC_VERSION)     return -1;
  if(reserved != 0)              return -1;
  if(msgtype > MSG_SYNC_ERR)     return -1;
  if(msglen < IPC_HDR_SIZE)      return -1;
  if(msglen > IPC_MAX_MSG)       return -1;
  c->body_need = (i32)(msglen - IPC_HDR_SIZE);
  c->msgtype   = msgtype;
  if(c->body_need > c->body_cap) {
    c->body = xrealloc(c->body, (size_t)c->body_need);
    c->body_cap = c->body_need;
  }
  return 0;
}

/* ---- send side ---- */

/* Write n bytes from buf to fd, retrying short writes and EINTR.
 * For non-blocking fds (clients + accepted), poll for writability on
 * EAGAIN. Returns 0 on success, -1 on error (caller should close). */
static int write_all(int fd, const void *buf, size_t n) {
  const char *p = buf;
  while(n > 0) {
    ssize_t w = sock_send(fd, p, n);
    if(w < 0) {
      int e = sock_lasterr();
      if(sock_intr(e)) continue;
      if(sock_would_block(e)) {
        struct pollfd pf = { fd, POLLOUT, 0 };
        int pr = sock_poll(&pf, 1, -1);
        if(pr < 0) {
          if(sock_intr(sock_lasterr())) continue;
          return -1;
        }
        if(pf.revents & (POLLERR|POLLHUP|POLLNVAL)) return -1;
        continue;
      }
      return -1;
    }
    p += w; n -= (size_t)w;
  }
  return 0;
}

/* Frame and send a message. payload may be NULL iff plen==0.
 * Returns 0 on success, -1 on error. */
static int send_frame(int fd, u8 msgtype, const void *payload, u32 plen) {
  u8  hdr[IPC_HDR_SIZE];
  u32 total = IPC_HDR_SIZE + plen;
  hdr[0] = IPC_ARCH_LE;
  hdr[1] = msgtype;
  hdr[2] = IPC_VERSION;
  hdr[3] = 0;
  memcpy(hdr + 4, &total, 4);
  if(write_all(fd, hdr, IPC_HDR_SIZE) < 0) return -1;
  if(plen && write_all(fd, payload, plen) < 0) return -1;
  return 0;
}

/* True iff a payload of plen bytes plus our header would exceed the cap.
 * Use u64 arithmetic so the IPC_HDR_SIZE add can't itself wrap. */
static int over_ipclimit(u64 plen) {
  return (plen + IPC_HDR_SIZE) > IPC_MAX_MSG;
}

/* Scratch buffer reused across all bd_-into-socket sends. Never freed;
 * grows monotonically. Eliminates the per-call alloc + page-fault cost
 * on the serialize output (the dominant overhead for large messages).
 * Single-threaded: the IPC layer runs on the main REPL thread on POSIX,
 * and forked children get their own copy via COW. */
static char *send_scratch;
static u64   send_scratch_cap;

/* Send the value `r` framed as msgtype. Consumes nothing. Returns 0/-1.
 * On bd_ failure or oversize payload, falls back to sending a sync-err
 * (the receiver will surface it as a kerror). */
static int send_value(int fd, u8 msgtype, K r) {
  u64 len = 0;
  K e = bd_into(r, &send_scratch, &send_scratch_cap, &len);
  /* bd_ can legitimately fail; over_ipclimit fires when the serialized
   * payload + 8-byte header would exceed our IPC frame cap. Both cases
   * are surfaced as a bd_-wrapped char-vector SYNC_ERR so the client
   * can uniformly db_ every payload regardless of msgtype. We rewrite
   * 'wsfull -> 'length on the IPC send path so any over-2GB payload
   * raises 'length. The recursive send_value is safe - it overwrites
   * the scratch with the small error string and we no longer need
   * the partial output. */
  if(E(e) || over_ipclimit(len)) {
    const char *m;
    if(!E(e))                          m = E[KERR_LENGTH];
    else if(e == KERR_WSFULL)          m = E[KERR_LENGTH];
    else if(e < EMAX)                  m = E[e];
    else                               m = "type";
    K str = tn(3, (i32)strlen(m));
    memcpy(px(str), m, strlen(m));
    int rc = send_value(fd, MSG_SYNC_ERR, str);
    _k(str);
    return rc;
  }
  return send_frame(fd, msgtype, send_scratch, (u32)len);
}

/* Format an error K into a plain C string, wrap it as a K char vector,
 * bd_-encode that, and send as MSG_SYNC_ERR. Receivers can therefore
 * uniformly db_ every payload regardless of msgtype. */
static int send_error(int fd, K err) {
  const char *m;
  if(err < EMAX) m = E[err];
  else           m = sk(err);
  size_t mlen = strlen(m);
  K str = tn(3, (i32)mlen);
  memcpy(px(str), m, mlen);
  int rc = send_value(fd, MSG_SYNC_ERR, str);
  _k(str);
  return rc;
}

/* ---- dispatch ---- */

/* Look up .m.s / .m.g / .m.c, returning a fresh reference (or an error K). */
static K get_handler(const char *name) {
  K nm = t(4, sp((char*)name));
  return scope_get(gs, nm);
}

/* Apply handler `h` to single argument `arg`. Consumes both. fne() takes
 * its arg as a *list of args*, so we wrap atomics/values in a 1-element
 * list before calling.
 *
 * We honor the user's \e setting so that handler errors behave exactly
 * like errors at the top-level REPL:
 *
 *   \e 1  pgreduce_ prints the error and drops into a debug sub-REPL.
 *         IPC dispatch pauses for all peers until `\` exits the sub-REPL.
 *   \e 0  fne returns the error silently; we print it here to match the
 *         REPL's "print and continue" behavior (the REPL's own call in
 *         repl.c does the same kprint after pgreduce returns).
 *
 * For .m.g (sync) the caller also gets the error via SYNC_ERR, so the
 * server-side print is in addition to the remote notification - same as
 * how the REPL would report an error that simultaneously got caught by
 * a trap handler. We still save+restore EFLAG so a handler that toggles
 * \e itself can't persistently change it. */
static K apply1(K h, K arg) {
  /* Guard against non-callable handlers. The user can put any K
   * value in .m.{s,g} (`.m.g:"{1+x}"` is a real footgun: the default
   * .m.g is `{. x}` which evals string *args*, but a string in the
   * handler slot itself isn't callable). fne() on a non-callable
   * crashes -- val() returns an error K for the same set, so we
   * use that as a fast and uniform check. Note: callable-but-wrong-
   * valence handlers (e.g. `.m.g:{x+y}`) project rather than crash;
   * we let those through and the peer receives the projection. */
  K vv = val(h);
  if(E(vv)) {
    _k(h);
    _k(arg);
    /* Match the post-fne print rule below: under \e 0, surface the
     * problem on the server too. Under \e 1 we stay silent (no
     * sub-repl: the offending dispatch was driven by an inbound
     * message, not a top-level expression, and we still need to
     * answer the peer with SYNC_ERR before unwinding). */
    if(!EFLAG) {
      K p = (vv < EMAX) ? kerror(E[vv]) : k_(vv);
      kprint(p, "", "\n", "");
    }
    return vv;
  }
  _k(vv);

  K args  = tn(0, 1);
  K *pa   = px(args);
  pa[0]   = arg;
  n(args) = 1;
  int e0  = EFLAG;
  K r     = fne(h, args, "");
  EFLAG   = e0;
  if(!e0 && E(r)) {
    K p = (r < EMAX) ? kerror(E[r]) : r;
    kprint(p, "", "\n", "");
    if(p != r) _k(p);
  }
  return r;
}

/* Mark the fd's conn as currently running a handler. Nested sync waits
 * exclude busy conns from their poll set so further data on the same
 * conn queues in the kernel until the handler returns. */
static void mark_in_dispatch(int fd, int v) {
  int ix = find_conn(fd);
  if(ix >= 0) conns[ix].in_dispatch = (u8)(v ? 1 : 0);
}

static void dispatch_async(int fd, K msg) {
  /* Async dispatch has no remote caller waiting on the result, so
   * handler errors can't be surfaced to the peer. Under \e 0 they're
   * silently dropped here; under \e 1 the user gets a debug sub-REPL
   * inside apply1 and the eventual return value is dropped on the
   * floor either way. */
  mark_in_dispatch(fd, 1);
  scope_set_z_w(fd);
  K h = get_handler(".m.s");
  if(E(h)) { scope_set_z_w(0); mark_in_dispatch(fd, 0); _k(msg); return; }
  K r = apply1(h, msg);
  scope_set_z_w(0);
  mark_in_dispatch(fd, 0);
  if(E(r)) { if(r >= EMAX) _k(r); }
  else _k(r);
}

static void dispatch_sync(int fd, K msg) {
  mark_in_dispatch(fd, 1);
  scope_set_z_w(fd);
  K h = get_handler(".m.g");
  if(E(h)) {
    scope_set_z_w(0);
    mark_in_dispatch(fd, 0);
    send_error(fd, h);
    if(h >= EMAX) _k(h);
    _k(msg);
    return;
  }
  K r = apply1(h, msg);
  scope_set_z_w(0);
  mark_in_dispatch(fd, 0);
  /* "abort" out of the handler means the user typed `\` to exit a
   * debug sub-REPL that was opened (under \e 1) for an error inside
   * the handler. The error itself was already printed and dismissed
   * by the user; forwarding the literal "abort" string would land
   * on the peer's top-level REPL where an abort-as-expression-value
   * fires help(0) (see repl.c). Treat it as "discarded" and answer
   * the peer with null instead. */
  if(E(r) && r >= EMAX && sk(r) == sp("abort")) {
    _k(r);
    send_value(fd, MSG_SYNC_RSP, null);
    return;
  }
  if(E(r)) {
    send_error(fd, r);
    if(r >= EMAX) _k(r);
  }
  else {
    send_value(fd, MSG_SYNC_RSP, r);
    _k(r);
  }
}

static void fire_close_handler(int fd) {
  mark_in_dispatch(fd, 1);
  scope_set_z_w(fd);
  K h = get_handler(".m.c");
  if(E(h)) { scope_set_z_w(0); mark_in_dispatch(fd, 0); return; }
  /* k semantics: .m.c is a string of K code that gets eval'd on close.
   * Anything that isn't a non-empty char vector (a lambda, sym, int,
   * list, the default "", ...) is silently ignored. The closed fd is
   * not passed; if the user wants it, they can capture it elsewhere
   * (e.g., maintain a global list inside .m.s). */
  if(T(h) != -3 || n(h) == 0) { scope_set_z_w(0); mark_in_dispatch(fd, 0); _k(h); return; }
  /* Evaluate by applying the canonical {. x} eval-lambda. Built fresh
   * each time (only fires on disconnect, so the cost is irrelevant)
   * so the user redefining .m.s can't affect us. */
  K eval_lam = fnnew("{. x}");
  if(E(eval_lam)) { scope_set_z_w(0); mark_in_dispatch(fd, 0); _k(h); return; }
  K r = apply1(eval_lam, h);
  scope_set_z_w(0);
  mark_in_dispatch(fd, 0);
  /* Close-callback errors have no caller; drop silently. */
  if(E(r)) { if(r >= EMAX) _k(r); }
  else _k(r);
}

/* Deserialize c->body in place (no K-vector wrap, no buffer adoption) and
 * dispatch. The body buffer stays owned by the conn so it can be reused
 * for the next message without re-allocating + re-faulting pages. */
static void deliver_message(ipc_conn *c) {
  int  fd      = c->fd;
  u8   msgtype = c->msgtype;
  i32  blen    = c->body_need;
  u8  *body    = c->body;

  K msg = (blen > 0) ? db_buf((const char*)body, (u64)blen)
                     : KERR_LENGTH;
  if(E(msg)) {
    /* Bad payload from peer. For a sync request we still owe the peer a
     * MSG_SYNC_ERR so it doesn't block. For other msgtypes we just drop;
     * any local sync waiter will be woken by the conn close that follows
     * if the stream is corrupt enough to matter. */
    if(msgtype == MSG_SYNC_REQ) {
      const char *m = (msg < EMAX) ? E[msg] : sk(msg);
      send_frame(fd, MSG_SYNC_ERR, m, (u32)strlen(m));
    }
    if(msg >= EMAX) _k(msg);
    return;
  }

  switch(msgtype) {
  case MSG_ASYNC:
    dispatch_async(fd, msg);
    break;
  case MSG_SYNC_REQ:
    dispatch_sync(fd, msg);
    break;
  case MSG_SYNC_RSP:
  case MSG_SYNC_ERR:
    if(c->waiting_sync && !c->sync_have) {
      c->sync_have     = 1;
      c->sync_is_err   = (msgtype == MSG_SYNC_ERR);
      c->sync_response = msg;
    }
    else {
      /* Stray response with no parked caller - drop. */
      _k(msg);
    }
    break;
  default:
    _k(msg);
    break;
  }
}

/* ---- namespace defaults ---- */

/* Set a global, taking ownership of `val`. Logs and frees on failure. */
static void set_global(const char *name, K val) {
  K nm = t(4, sp((char*)name));
  K r  = scope_set(gs, nm, val);
  if(E(r)) {
    const char *m = (r < EMAX) ? E[r] : sk(r);
    fprintf(stderr,"ipc: ipc_init_ns: setting %s failed: %s\n", name, m);
    if(r >= EMAX) _k(r);
    /* scope_set consumes val on error too; nothing else to free */
  }
  else _k(r);
}

void ipc_init_ns(void) {
  /* Defaults match the user-facing spec:
   *   .m.s:{. x}   async handler  (eval the message; discard result)
   *   .m.g:{. x}   sync  handler  (eval; result becomes the response)
   *   .m.c:""      close handler  (string -> no-op when fired)
   * Built directly with K APIs (mirroring how scope_init wires up .k/.z),
   * so we don't need a working parser or a writable source buffer. */
  K s_lam = fnnew("{. x}");
  K g_lam = fnnew("{. x}");
  K c_str = tn(3, 0);    /* empty char vector */
  if(E(s_lam) || E(g_lam) || E(c_str)) {
    fprintf(stderr,"ipc: ipc_init_ns: failed to build defaults\n");
    if(!E(s_lam)) _k(s_lam);
    if(!E(g_lam)) _k(g_lam);
    if(!E(c_str)) _k(c_str);
    return;
  }
  set_global(".m.s", s_lam);
  set_global(".m.g", g_lam);
  set_global(".m.c", c_str);
}

/* Drive the receive state machine for one ready fd.
 * Returns 0 to keep the connection, -1 to close it. */
static int recv_step(ipc_conn *c) {
  for(;;) {
    /* phase 1: header */
    if(c->hdr_have < IPC_HDR_SIZE) {
      ssize_t r = sock_recv(c->fd, c->hdr + c->hdr_have,
                                   IPC_HDR_SIZE - c->hdr_have);
      if(r == 0) return -1;                 /* peer closed */
      if(r <  0) {
        int e = sock_lasterr();
        if(sock_intr(e))                    continue;
        if(sock_would_block(e))             return 0;
        return -1;                          /* close conn; sync waiter (if any) gets 'conn' */
      }
      c->hdr_have += (i32)r;
      if(c->hdr_have < IPC_HDR_SIZE) return 0;  /* need more, wait for poll */
      if(parse_header(c) < 0) return -1;
      if(c->body_need == 0) {
        deliver_message(c);
        reset_recv(c);
        continue;                              /* try to drain another msg */
      }
    }

    /* phase 2: body */
    ssize_t r = sock_recv(c->fd, c->body + c->body_have,
                                 (size_t)(c->body_need - c->body_have));
    if(r == 0) return -1;
    if(r <  0) {
      int e = sock_lasterr();
      if(sock_intr(e))                    continue;
      if(sock_would_block(e))             return 0;
      return -1;
    }
    c->body_have += (i32)r;
    if(c->body_have < c->body_need) return 0;   /* wait for more */

    /* full message in hand */
    deliver_message(c);
    reset_recv(c);
    /* loop and try to drain another message that may already be buffered */
  }
}

void ipc_handle_pollfd(int fd, short revents) {
#ifdef FUZZING
  /* fuzz: ipc_extra_pollfds returns 0, so this is unreachable. Defensive. */
  (void)fd; (void)revents; return;
#else
  if(fd == listen_iter_fd) {
    if(revents & POLLIN) handle_accept(fd, IPC_MODE_ITER);
    return;
  }
  if(fd == listen_fork_fd) {
    if(revents & POLLIN) handle_accept(fd, IPC_MODE_FORK);
    return;
  }
  int idx = find_conn(fd);
  if(idx < 0) return;

  if(revents & POLLIN) {
    if(recv_step(&conns[idx]) < 0) { close_conn_at(idx); return; }
  }
  if(revents & (POLLHUP|POLLERR)) {
    close_conn_at(idx);
  }
#endif
}

/* ---- client side: open / close / send ---- */

/* Wrap a socket error code as a kerror with a leading prefix, e.g.
 * "conn: Connection refused". `err` is whatever sock_lasterr() returned;
 * sock_errstr translates it (errno on POSIX, FormatMessage on Win32). */
static K cerror(const char *prefix, int err) {
  const char *s = sock_errstr(err);
  size_t pn = strlen(prefix), sn = strlen(s);
  char *b = xmalloc(pn + sn + 3);
  memcpy(b, prefix, pn); b[pn]=':'; b[pn+1]=' ';
  memcpy(b + pn + 2, s, sn); b[pn + 2 + sn] = 0;
  K r = kerror(b);
  xfree(b);
  return r;
}

/* Extract host as a NUL-terminated C string into buf (size >= bufsz).
 * Accepts sym (T==4) or char vector (T==-3). An empty host (`` ` `` or "")
 * is treated as "localhost", matching k. Returns 0 on success, -1 on
 * type/length error. */
static int host_cstr(K host, char *buf, size_t bufsz) {
  const char *s = NULL; size_t l = 0;
  if(T(host) == 4)        { s = sk(host); l = strlen(s); }
  else if(T(host) == -3)  { s = px(host); l = (size_t)n(host); }
  else                    return -1;
  if(l == 0) {
    /* empty host -> localhost */
    if(sizeof "localhost" > bufsz) return -1;
    memcpy(buf, "localhost", sizeof "localhost");
    return 0;
  }
  if(l + 1 > bufsz) return -1;
  memcpy(buf, s, l);
  buf[l] = 0;
  return 0;
}

/* Look up an existing client conn that matches (host, port). Returns
 * fd on hit, -1 on miss. Used to dedup repeated 3:(host;port) calls. */
static int find_client_conn(const char *host, i32 port) {
  for(int i = 0; i < conn_count; i++) {
    ipc_conn *c = &conns[i];
    if(c->is_client && c->port == port && !strcmp(c->host, host))
      return c->fd;
  }
  return -1;
}

K ipc_open(K hp) {
#ifdef FUZZING
  /* fuzz: never connect (no DNS, no socket, no surprise local services) */
  (void)hp; return KERR_DOMAIN;
#else
  if(T(hp) != 0)   return KERR_TYPE;
  if(n(hp) != 2)   return KERR_LENGTH;
  K *pk = px(hp);
  K host = pk[0], port = pk[1];
  if(T(port) != 1) return KERR_TYPE;
  i32 p = ik(port);
  if(p <= 0 || p > 65535) return KERR_DOMAIN;

  char hbuf[256];
  if(host_cstr(host, hbuf, sizeof hbuf) < 0) return KERR_TYPE;

  /* Dedup: if we already have a client conn for this (host, port), return
   * its fd. 3:(`;5555) typed three times reuses the same socket; the
   * /proc check confirms we don't burn a new fd. */
  int existing = find_client_conn(hbuf, p);
  if(existing >= 0) return t(1, (u32)existing);

  /* Resolve. AF_INET only for now; IPv4 dotted-quad or hostname both work. */
  struct addrinfo hints, *res = 0;
  memset(&hints, 0, sizeof hints);
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  char pbuf[8]; snprintf(pbuf, sizeof pbuf, "%d", (int)p);
  int gai = getaddrinfo(hbuf, pbuf, &hints, &res);
  if(gai != 0 || !res) {
    /* getaddrinfo errors are not errno values; gai_strerror them inline. */
    const char *gs = gai_strerror(gai);
    size_t gn = strlen(gs);
    char *b = xmalloc(gn + 8);  /* "value: " (7) + gs (gn) + NUL (1) */
    memcpy(b, "value: ", 7);
    memcpy(b + 7, gs, gn + 1);
    K r = kerror(b);
    xfree(b);
    if(res) freeaddrinfo(res);
    return r;
  }

  sock_t fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if(fd == INVALID_SOCK) {
    K r = cerror("conn", sock_lasterr());
    freeaddrinfo(res);
    return r;
  }
  if(connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
    K r = cerror("conn", sock_lasterr());
    sock_close(fd);
    freeaddrinfo(res);
    return r;
  }
  freeaddrinfo(res);

  if(conn_count >= MAX_CONNS) {
    sock_close(fd);
    return kerror("conn: too many connections");
  }
  /* Non-blocking AFTER connect so we don't have to deal with EINPROGRESS. */
  (void)sock_setnb(fd);
  sock_nodelay(fd);

  ipc_conn *c = &conns[conn_count++];
  memset(c, 0, sizeof *c);
  c->fd        = (int)fd;
  c->is_client = 1;
  c->port      = p;
  /* hbuf is at most 255 bytes (host_cstr enforces); host[256] room incl NUL */
  memcpy(c->host, hbuf, strlen(hbuf) + 1);
  return t(1, (u32)fd);
#endif
}

K ipc_close(int fd) {
#ifdef FUZZING
  /* fuzz: no client conns exist; reject without touching any fd */
  (void)fd; return KERR_DOMAIN;
#else
  int idx = find_conn(fd);
  if(idx < 0)              return kerror("conn: bad handle");
  if(!conns[idx].is_client) return kerror("conn: not a client handle");
  close_conn_at(idx);
  return null;
#endif
}

K ipc_send_async(int fd, K msg) {
#ifdef FUZZING
  /* fuzz: nothing to send to */
  (void)fd; (void)msg; return KERR_DOMAIN;
#else
  int idx = find_conn(fd);
  if(idx < 0)              return kerror("conn: bad handle");
  if(!conns[idx].is_client) return kerror("conn: not a client handle");
  u64 len = 0;
  K e = bd_into(msg, &send_scratch, &send_scratch_cap, &len);
  /* Rewrite 'wsfull -> 'length on the IPC send path to match k. */
  if(E(e)) return (e == KERR_WSFULL) ? KERR_LENGTH : e;
  if(over_ipclimit(len)) return KERR_LENGTH;
  int rc = send_frame(fd, MSG_ASYNC, send_scratch, (u32)len);
  if(rc < 0) {
    int last = sock_lasterr();
    int idx2 = find_conn(fd);
    if(idx2 >= 0) close_conn_at(idx2);
    return cerror("conn", last);
  }
  return null;
#endif
}

K ipc_send_sync(int fd, K msg) {
#ifdef FUZZING
  /* fuzz: CRITICAL - real impl blocks on poll(); would hang the fuzzer */
  (void)fd; (void)msg; return KERR_DOMAIN;
#else
  int idx = find_conn(fd);
  if(idx < 0)              return kerror("conn: bad handle");
  if(!conns[idx].is_client) return kerror("conn: not a client handle");
  ipc_conn *c = &conns[idx];
  if(c->waiting_sync) return kerror("conn: sync re-entry on same handle");

  u64 len = 0;
  K e = bd_into(msg, &send_scratch, &send_scratch_cap, &len);
  /* Rewrite 'wsfull -> 'length on the IPC send path to match k. */
  if(E(e)) return (e == KERR_WSFULL) ? KERR_LENGTH : e;
  if(over_ipclimit(len)) return KERR_LENGTH;
  int rc = send_frame(fd, MSG_SYNC_REQ, send_scratch, (u32)len);
  if(rc < 0) {
    int last = sock_lasterr();
    if(find_conn(fd) >= 0) close_conn_at(find_conn(fd));
    return cerror("conn", last);
  }

  /* Park on this fd until SYNC_RSP/ERR (or the conn dies).
   *
   * We poll the same set the repl polls: this fd, the listen socket, and
   * any other open conns. That way, while we wait for our own answer,
   * other peers can still send us messages and our .m.{s,g} handlers
   * will run inline (which is fine: deliver_message dispatches them
   * directly; only SYNC_RSP/ERR for *this* fd terminates our wait). */
  c->waiting_sync = 1;
  c->sync_have    = 0;
  c->sync_response = 0;
  c->sync_is_err  = 0;

  K result = 0;
  for(;;) {
    /* recheck every loop: close_conn_at could have removed us */
    int ix = find_conn(fd);
    if(ix < 0) {
      result = kerror("conn: closed");
      break;
    }
    ipc_conn *cc = &conns[ix];
    if(cc->sync_have) {
      K v = cc->sync_response;
      u8 is_err = cc->sync_is_err;
      cc->sync_have = 0;
      cc->sync_response = 0;
      cc->sync_is_err = 0;
      cc->waiting_sync = 0;
      if(is_err) {
        /* SYNC_ERR payload was bd_-wrapped char vector */
        if(T(v) == -3) {
          char *p = px(v);
          size_t l = (size_t)n(v);
          char *b = xmalloc(l + 1);
          memcpy(b, p, l); b[l] = 0;
          result = kerror(b);
          xfree(b);
          _k(v);
        }
        else {
          /* unexpected shape; pass through whatever we got as a kerror */
          result = kerror("remote error");
          _k(v);
        }
      }
      else result = v;
      break;
    }

    struct pollfd pfd[POLL_BATCH];
    int n_ipc = ipc_extra_pollfds(pfd, POLL_BATCH);
    int pr = sock_poll(pfd, n_ipc, -1);
    if(pr < 0) {
      if(sock_intr(sock_lasterr())) continue;
      cc->waiting_sync = 0;
      result = cerror("conn", sock_lasterr());
      break;
    }
    for(int i = 0; i < n_ipc; i++) {
      if(pfd[i].revents) ipc_handle_pollfd(pfd[i].fd, pfd[i].revents);
    }
  }
  return result;
#endif
}

/* Process-exit cleanup. Closes the listen socket and every live conn
 * (frees per-conn body buffers), releases the persistent send scratch,
 * and on Windows releases the stdin event + Winsock. Does NOT fire
 * .m.c handlers: the process is exiting, K state is about to be torn
 * down, and any remote peers will observe the close as a normal
 * disconnect. Idempotent. */
void ipc_shutdown(void) {
  for(int i = 0; i < conn_count; i++) {
    ipc_conn *c = &conns[i];
    sock_close(c->fd);
    if(c->body) { xfree(c->body); c->body = NULL; c->body_cap = 0; }
    if(c->sync_have && c->sync_response && !E(c->sync_response)) {
      _k(c->sync_response);
      c->sync_response = 0;
      c->sync_have     = 0;
    }
  }
  conn_count = 0;
  if(listen_iter_fd >= 0) { sock_close(listen_iter_fd); listen_iter_fd = -1; listen_iter_port = 0; }
  if(listen_fork_fd >= 0) { sock_close(listen_fork_fd); listen_fork_fd = -1; listen_fork_port = 0; }
  if(send_scratch) {
    xfree(send_scratch);
    send_scratch     = NULL;
    send_scratch_cap = 0;
  }
#ifdef _WIN32
  /* Reader thread is blocked in ReadFile(stdin); we can't unblock it
   * cleanly without closing the console handle. Let process exit drop it. */
  if(stdin_event) { CloseHandle(stdin_event); stdin_event = NULL; }
  WSACleanup();
#endif
}
