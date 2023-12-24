#ifdef _WIN32
#include <stdio.h>
#include "unistd.h"
#else
#include <unistd.h>
#endif
#include "interp.h"

int main(int argc, char **argv) {
  int interactive = isatty(fileno(stdin));
  if(interactive) fprintf(stderr, "gk v004rc\n\n");
  kinit();
  if(argc>1) interp(argv[1],interactive,1);
  else interp(0,interactive,1);
  return 0;
}
