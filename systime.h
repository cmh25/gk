#ifndef SYSTIME_H
#define SYSTIME_H 1
/* https://stackoverflow.com/questions/10905892/equivalent-of-gettimeofday-for-windows */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h> // portable: uint64_t   MSVC: __int64

// MSVC defines this in winsock2.h!?
typedef struct timeval {
    long tv_sec;
    long tv_usec;
} timeval;

int gettimeofday(struct timeval * tp, struct timezone * tzp);

#endif /* SYSTIME_H */
