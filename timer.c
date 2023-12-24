#include "timer.h"
#include <stdio.h>
#ifdef _WIN32
#include "systime.h"
#else
#include <sys/time.h>
#endif

static struct timeval tvs;
static struct timeval tve;

void timer_start() {
  gettimeofday(&tvs,0);
}

double timer_stop() {
  gettimeofday(&tve,0);
  double d = tve.tv_sec*1000000+tve.tv_usec-tvs.tv_sec*1000000-tvs.tv_usec;
  d /= 1000; /* us to ms */
  return d;
}
