#ifndef TIMEOUT_READ_H
#define TIMEOUT_READ_H

#include <stdlib.h>
#include <unistd.h>

ssize_t timeout_read(
    int fd,
    char *buffer,
    size_t size,
    long long timeout_usec
);

#endif
