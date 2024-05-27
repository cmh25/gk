#ifdef _WIN32
  #include "unistd.h"
#else
  #include <unistd.h>
#endif
#include "interp.h"

int main(int argc, char **argv) {
  if(isatty(fileno(stdin))) fprintf(stderr, "gk-v0.0.1-rc\n\n");
  kinit();
  if(argc>1) load(argv[1]);
  interp(1);
  return 0;
}
