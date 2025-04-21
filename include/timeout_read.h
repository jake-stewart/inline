#ifndef TIMEOUT_READ_H
#define TIMEOUT_READ_H

#include <stdlib.h>
#include <unistd.h>

#define TIMEOUT_READ_TIMEOUT -1
#define TIMEOUT_READ_ERROR -2

ssize_t timeout_read(
    int fd,
    char *buffer,
    size_t size,
    long long timeout_usec
);

#endif
