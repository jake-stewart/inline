#ifndef FD_H
#define FD_H

#include <sys/select.h>
#include <unistd.h>
#include <stdbool.h>

typedef enum {
    FD_READ = 0b001,
    FD_WRITE = 0b010,
    FD_EXCEPT = 0b100,
    FD_ALL = 0b111,
} fd_event_type;

typedef struct {
    fd_event_type type;
    int fd;
    bool timed_out;
    void *data;
} fd_event;

typedef int (*fd_event_callback)(fd_event);

void fd_subscribe(
    fd_event_type type,
    int fd,
    fd_event_callback,
    void *data
);
void fd_unsubscribe(fd_event_type type, int fd);

void fd_clear_timeout(fd_event_type type, int fd);
void fd_set_timeout(fd_event_type type, int fd, long long timeout_usec);

bool fd_has_subscriptions();
int fd_loop();

#endif
