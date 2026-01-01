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
  fprintf(stderr,"usage: %s [-q] [script]\n",s);
  exit(1);
}

int main(int argc, char **argv) {
  K r=0;
  int i,quiet=0;
  char *a,*script=0;
  for(i=1;i<argc;++i) {
    a=argv[i];
    if(a[0]=='-') {
      if(!a[1]) usage(argv[0]);
      else if(!strcmp(a,"-q")) quiet=1;
      else usage(argv[0]);
    }
    else { script=a; break; }
  }
  setvbuf(stdout, NULL, _IONBF, 0);
  if(!quiet) fprintf(stderr, "gk-v1.0.0 Copyright (c) 2023-2026 Charles Hall\n\n");
#ifdef _WIN32
  SetConsoleCtrlHandler(ctlc,TRUE);
#else
  signal(SIGINT,ctlc);
#endif
  kinit();
  scope_init(argc,argv);
  fninit();
  pinit();
  if(script) {
    r=load(script,1);
    if(E(r)) kprint(r,"","\n","");
    if(EAL) exit__(t(1,0));
    if(EXIT) exit__(EXIT);
  }
  repl();
  return 0;
}
