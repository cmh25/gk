#ifndef SYSTIME_H
#define SYSTIME_H 1

/* https://stackoverflow.com/questions/10905892/equivalent-of-gettimeofday-for-windows */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <time.h>     /* MinGW-w64 declares struct timeval here (via _timeval.h);
                         self-contained so includers needn't pull it in first. */

/* MinGW-w64's <time.h> defines struct timezone in newer header sets (e.g. MSYS2
   CLANG64), guarded by _TIMEZONE_DEFINED, but older ones (UCRT64/MINGW64 at time
   of writing) don't, and MSVC never does.  Mirror the mingw guard: define it only
   when absent, and mark it defined so any later system header skips it. */
#ifndef _TIMEZONE_DEFINED
#define _TIMEZONE_DEFINED
struct timezone {
  int32_t tz_minuteswest;
  int32_t tz_dsttime;
};
#endif

/* MSVC's <time.h> has no struct timeval, so define it.  MinGW-w64 DOES provide
   it (via <time.h> -> _timeval.h), where redefining the struct is a hard error
   (unlike an identical typedef) -- so skip it there and use the system's. */
#if !defined(__MINGW32__)
typedef struct timeval {
  int64_t tv_sec;
  int32_t tv_usec;
} timeval;
#endif

int __gettimeofday(struct timeval * tp, struct timezone * tzp);

#endif /* SYSTIME_H */
