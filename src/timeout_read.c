#include "timeout_read.h"
#include "usec.h"
#include <sys/select.h>

ssize_t timeout_read(int fd, char *buffer, size_t size, long long timeout_usec) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    struct timeval timeout = usec_to_timeval(timeout_usec);
    ssize_t status = select(fd + 1, &set, NULL, NULL, &timeout);
    if (status <= 0) {
        return status ? TIMEOUT_READ_ERROR : TIMEOUT_READ_TIMEOUT;
    }
    status = read(fd, buffer, size);
    if (status < 0) {
        return TIMEOUT_READ_ERROR;
    }
    return status;
}
