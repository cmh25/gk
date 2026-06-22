#ifndef IPC_H
#define IPC_H

/* IPC over TCP for gk.
 *
 * This module owns:
 *   - the (single) listening socket
 *   - the table of active connection fds (server-accepted + client-opened)
 *   - parsing/framing/dispatch of incoming messages
 *   - the client-side open/close/send primitives invoked by 3:/4:
 *
 * Server (listen) path:
 *   The repl drives polling (see repl.c::poll_getc). At each idle prompt the
 *   repl asks ipc_extra_pollfds() to fill in extra fds, then for any fd that
 *   fires it calls ipc_handle_pollfd(). Complete framed messages are
 *   db_-deserialized and dispatched to .m.s (async), .m.g (sync request),
 *   or .m.c (close); the result of .m.g is framed back to the peer as a
 *   sync response/error.
 *
 * Client path:
 *   3:(host;port)   ipc_open  -> int handle (the socket fd)
 *   3:w             ipc_close (w must be an open client handle)
 *   w 3:msg         ipc_send_async (returns null)
 *   w 4:msg         ipc_send_sync  (blocks for response, returns deserialized value)
 *
 * Wire format: see comment in ipc.c.
 */

#include "k.h"

struct pollfd;

/* One-time platform setup (WSAStartup on Windows, no-op on POSIX).
 * Returns 0 on success, -1 on failure. Call once at startup, before
 * any other ipc_* function. */
int  ipc_init(void);

/* Counterpart to ipc_init. Optional - the OS reclaims sockets on exit. */
void ipc_shutdown(void);

/* Spin up the background console-reader thread that lets repl_getc() block
 * on stdin AND on IPC sockets at the same time (Win32 only; POSIX is
 * a no-op since poll() handles both natively). Call once after ipc_init.
 * Returns 0 on success, -1 on failure. */
int  ipc_stdin_init(void);

#ifdef _WIN32
/* Pop one byte from the stdin reader thread, blocking on stdin or IPC
 * events. Returns the byte (0..255) or EOF (-1). Win32 only. */
int  ipc_stdin_getc(void);
#endif

/* Listener modes. */
#define IPC_MODE_ITER 0   /* inline: dispatch on the parent repl      */
#define IPC_MODE_FORK 1   /* forking: fork() per accepted conn (POSIX)*/

/* Start (or stop) the listening socket on the given port and mode.
 *   port > 0 : bind+listen on 0.0.0.0:port; replaces any existing listener
 *   port == 0: stop listening (mode is ignored)
 *   mode     : IPC_MODE_ITER or IPC_MODE_FORK
 * Returns 0 on success, or a K error (e.g., "bind: Address already
 * in use", or "fork: not supported on windows" if mode==FORK on Win32). */
K ipc_listen(int port, int mode);

/* Listen port for the given mode (IPC_MODE_ITER or IPC_MODE_FORK),
 * or 0 if that slot is not currently bound. */
int ipc_listen_port_for(int mode);

/* Fill in up to `max` extra pollfd entries with (listen_fd | client_fds).
 * Returns the number written. Each entry has events=POLLIN and revents=0. */
int ipc_extra_pollfds(struct pollfd *fds, int max);

/* Handle one fd that fired in the poll() set. */
void ipc_handle_pollfd(int fd, short revents);

/* Install .m.{s,g,c} defaults in the global namespace. Must be called
 * after pinit() (so the parser is alive) and after scope_init(). */
void ipc_init_ns(void);

/* ---- client side (called from io.c by 3:/4:) ---- */

/* Open a TCP client connection. `hp` must be a 2-element list (host;port)
 * where host is a sym or char vector and port is an int. On success returns
 * an int K wrapping the socket fd; on failure returns a K error
 * (e.g., kerror("conn: Connection refused")). */
K ipc_open(K hp);

/* Close a client fd previously returned from ipc_open. Returns null, or a
 * K error if `fd` is not a recognized client handle. */
K ipc_close(int fd);

/* Frame and send msg on fd as MSG_ASYNC. Returns null or K error. */
K ipc_send_async(int fd, K msg);

/* Frame and send msg on fd as MSG_SYNC_REQ; block until MSG_SYNC_RSP/ERR
 * arrives on the same fd. Returns the deserialized response value, or a
 * K error (peer closed mid-call -> 'conn'; remote raised -> the remote
 * error string). */
K ipc_send_sync(int fd, K msg);

#endif /* IPC_H */
