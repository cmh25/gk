#ifdef _WIN32
#include <stdio.h>
#include "unistd.h"
#else
#include <unistd.h>
#endif
#include "interp.h"

int main(int argc, char **argv) {
  K *r;
  if(isatty(fileno(stdin))) fprintf(stderr, "gk v004rc\n\n");
  kinit();
  if(argc>1) r=load(argv[1]);
  interp(1);
  return 0;
}
