#include "usec.h"
#include <stddef.h>

#define MSEC_IN_SEC 1000
#define USEC_IN_MSEC 1000
#define USEC_IN_SEC (USEC_IN_MSEC * MSEC_IN_SEC)

long long timeval_to_usec(struct timeval tv) {
    return tv.tv_sec * USEC_IN_SEC + tv.tv_usec;
}

struct timeval usec_to_timeval(long long usec) {
    struct timeval tv;
    tv.tv_sec = usec / USEC_IN_SEC;
    tv.tv_usec = usec % USEC_IN_SEC;
    return tv;
}

long long usec_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return timeval_to_usec(tv);
}
