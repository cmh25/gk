#include "repl.h"
#include "k.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scope.h"
#include "fn.h"
#ifdef _WIN32
#include "unistd.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#endif
#include "b.h"
#include "ipc.h"

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
  int i,quiet=0,iter_port=0,fork_port=0;
  char *a,*script=0;
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
    else { script=a; break; }
  }
  setvbuf(stdout, NULL, _IONBF, 0);
  if(!quiet) fprintf(stderr, "gk-v1.1.0 Copyright (c) 2023-2026 Charles Hall\n\n");
#ifdef _WIN32
  SetConsoleCtrlHandler(ctlc,TRUE);
#else
  signal(SIGINT,ctlc);
  signal(SIGPIPE,SIG_IGN);  /* ipc: don't die when writing to a closed peer */
#endif
  kinit();
  scope_init(argc,argv);
  fninit();
  pinit();
  if(ipc_init()<0) { fprintf(stderr,"gk: ipc_init failed\n"); exit(1); }
  if(ipc_stdin_init()<0) { fprintf(stderr,"gk: ipc_stdin_init failed\n"); exit(1); }
  ipc_init_ns();
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
  return 0;
}
