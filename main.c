#ifdef _WIN32
  #include "unistd.h"
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <unistd.h>
  #include <signal.h>
#endif
#include "interp.h"

#ifdef _WIN32
BOOL WINAPI ctlc(DWORD t) { if(t==CTRL_C_EVENT) { S=1; return TRUE; } return FALSE; }
#else
void ctlc(int n) { (void)n; S=1; }
#endif

int main(int argc, char **argv) {
  if(isatty(fileno(stdin))) fprintf(stderr, "gk-v0.0.1-rc Copyright (c) 2023-2024 Charles Hall\n\n");
  #ifdef _WIN32
  SetConsoleCtrlHandler(ctlc,TRUE);
  #else
  signal(SIGINT,ctlc);
  #endif
  kinit(argc,argv);
  if(argc>1) load(argv[1]);
  interp(1);
  return 0;
}
