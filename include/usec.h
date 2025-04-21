#ifndef USEC_H
#define USEC_H

#include <sys/time.h>

long long timeval_to_usec(struct timeval tv);
struct timeval usec_to_timeval(long long usec);
long long usec_timestamp();

#endif
