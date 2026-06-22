#include "repl.h"
#include "k.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scope.h"
#include "fn.h"
#ifdef _WIN32
#include "win_unistd.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>      /* _setmode, _fileno */
#include <fcntl.h>   /* _O_BINARY */
#else
#include <unistd.h>
#include <signal.h>
#endif
#include "b.h"
#include "ipc.h"
#include "tmr.h"

#ifdef FUZZING
#define EAL 1
#else
#define EAL 0
#endif

#ifdef _WIN32
BOOL WINAPI ctlc(DWORD t) { if(t==CTRL_C_EVENT) { STOP=1; return TRUE; } return FALSE; }
#else
void ctlc(int n) { (void)n; STOP=1; }
#endif

static void usage(char *s) {
  fprintf(stderr,"usage: %s [-q] [-i PORT] [-f PORT] [script]\n",s);
  exit(1);
}

int main(int argc, char **argv) {
  K r=0;
  int i,quiet=0,iter_port=0,fork_port=0,nargs=0;
  char *a,*script=0,**args;
#ifdef _WIN32
  /* gk prints LF, never CRLF: put stdout/stderr in binary mode so the Windows
   * CRT doesn't translate '\n' -> '\r\n'.  gk's output is then byte-identical on
   * every platform, and binary data written to stdout isn't corrupted.  (The
   * console renders a bare '\n' as a proper newline, so interactive output and
   * the REPL are unaffected -- this only stops the CRLF rewrite.) */
  _setmode(_fileno(stdout), _O_BINARY);
  _setmode(_fileno(stderr), _O_BINARY);
#endif
  /* .z.i collects every non-flag token after the script.  Flags (-q/-i/-f)
   * are consumed wherever they appear, so flag position is irrelevant */
  args=malloc((argc>1?argc:1)*sizeof(*args));
  if(!args) { fprintf(stderr,"gk: out of memory\n"); exit(1); }
  for(i=1;i<argc;++i) {
    a=argv[i];
    if(a[0]=='-') {
      if(!a[1]) usage(argv[0]);
      else if(!strcmp(a,"-q")) quiet=1;
      else if(!strcmp(a,"-i") || !strcmp(a,"-f")) {
        /* -i PORT: inline ipc listener
         * -f PORT: forking ipc listener (POSIX only)
         * Both may be given; each may appear at most once. */
        if(i+1>=argc) usage(argv[0]);
        int is_fork = (a[1]=='f');
        ++i;
        char *end=0;
        long v=strtol(argv[i],&end,10);
        if(!end||*end||v<1||v>65535) usage(argv[0]);
        if(is_fork) { if(fork_port) usage(argv[0]); fork_port=(int)v; }
        else        { if(iter_port) usage(argv[0]); iter_port=(int)v; }
      }
      else usage(argv[0]);
    }
    else if(!script) script=a;  /* first non-flag token is the script */
    else args[nargs++]=a;       /* remaining non-flags are user args (.z.i) */
  }
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);  /* glibc has this by default; Windows pipes
    don't -- without it, buffered stderr (prompts/errors) races the unbuffered
    stdout (results) and the merged transcript reorders intermittently */
  if(!quiet) fprintf(stderr, "gk-v3.0.0 Copyright (c) 2023-2026 Charles Hall\n\n");
#ifdef _WIN32
  SetConsoleCtrlHandler(ctlc,TRUE);
#else
  signal(SIGINT,ctlc);
  signal(SIGPIPE,SIG_IGN);  /* ipc: don't die when writing to a closed peer */
#endif
  kinit();
  scope_init(args,nargs);
  free(args);
  fninit();
  pinit();
  if(ipc_init()<0) { fprintf(stderr,"gk: ipc_init failed\n"); exit(1); }
  if(ipc_stdin_init()<0) { fprintf(stderr,"gk: ipc_stdin_init failed\n"); exit(1); }
  ipc_init_ns();
  tmr_init();
  if(iter_port) {
    K e=ipc_listen(iter_port,IPC_MODE_ITER);
    if(e) {
      const char *m = (e < EMAX) ? E[e] : sk(e);
      fprintf(stderr,"gk: -i %d: %s\n",iter_port,m);
      if(e>=EMAX) _k(e);
      exit(1);
    }
  }
  if(fork_port) {
    K e=ipc_listen(fork_port,IPC_MODE_FORK);
    if(e) {
      const char *m = (e < EMAX) ? E[e] : sk(e);
      fprintf(stderr,"gk: -f %d: %s\n",fork_port,m);
      if(e>=EMAX) _k(e);
      exit(1);
    }
  }
  if(script) {
    r=load(script,1);
    if(E(r)) kprint(r,"","\n","");
    if(EAL) exit__(t(1,0));
    if(EXIT) exit__(EXIT);
  }
  repl();
  ipc_shutdown();
  tmr_shutdown();
  return 0;
}
