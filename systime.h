#ifndef SYSTIME_H
#define SYSTIME_H 1

/* https://stackoverflow.com/questions/10905892/equivalent-of-gettimeofday-for-windows */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

struct timezone {
  int32_t tz_minuteswest;
  int32_t tz_dsttime;
};

typedef struct timeval {
  int64_t tv_sec;
  int32_t tv_usec;
} timeval;

int __gettimeofday(struct timeval * tp, struct timezone * tzp);

#endif /* SYSTIME_H */
