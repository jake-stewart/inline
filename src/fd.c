#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "fd.h"
#include "utils.h"
#include "vec.h"
#include "usec.h"
#include "error.h"

typedef struct {
    int fd;
    fd_event_callback callback;
    long long timeout;
    long long timeout_remaining;
    void *data;
} FdSubscription;

typedef struct {
    fd_event_type type;
    vec(FdSubscription) subscriptions;
    fd_set fds;
} FdSubject;

bool active = false;
FdSubject subjects[3] = {
    { FD_EXCEPT, NULL, { } },
    { FD_READ, NULL, { } },
    { FD_WRITE, NULL, { } }
};

#define FOR_EACH_FD_SUBJECT(types, idx, set, code) \
    for (int idx = 0; idx < 3; idx++) { \
        FdSubject *set = &subjects[idx]; \
        if (set->type & types) { \
            code; \
        } \
    }

void fd_subscribe(
    fd_event_type types,
    int fd,
    fd_event_callback callback,
    void *data
) {
    FdSubscription sub;
    sub.fd = fd;
    sub.timeout = -1;
    sub.callback = callback;
    sub.data = data;
    FOR_EACH_FD_SUBJECT(types, idx, set, {
        if (!set->subscriptions) {
            set->subscriptions = vec_new();
        }
        FdSubscription *existing =
            vec_find(set->subscriptions, i, existing, ({
                existing.fd == fd;
            }));
        if (existing) {
            *existing = sub;
        }
        else {
            vec_push(set->subscriptions, sub);
        }
    });
}

void fd_unsubscribe(fd_event_type types, int fd) {
    FOR_EACH_FD_SUBJECT(types, idx, set, {
        if (set->subscriptions) {
            if (active) {
                vec_each_ptr(set->subscriptions, i, sub, {
                    if (sub->fd == fd) {
                        sub->fd = -1;
                        break;
                    }
                });
            }
            else {
                vec_each(set->subscriptions, i, sub, {
                    if (sub.fd == fd) {
                        vec_remove_at(set->subscriptions, i);
                        if (!vec_len(set->subscriptions)) {
                            vec_free(set->subscriptions);
                            set->subscriptions = NULL;
                        }
                        break;
                    }
                });
            }
        }
    });
}

void fd_clear_timeout(fd_event_type types, int fd) {
    FOR_EACH_FD_SUBJECT(types, idx, set, {
        if (set->subscriptions) {
            vec_each_ptr(set->subscriptions, i, sub, {
                if (sub->fd == fd) {
                    sub->timeout = -1;
                    break;
                }
            });
        }
    });
}

void fd_set_timeout(fd_event_type types, int fd, long long timeout_usec) {
    FOR_EACH_FD_SUBJECT(types, idx, set, {
        if (set->subscriptions) {
            vec_each_ptr(set->subscriptions, i, sub, {
                if (sub->fd == fd) {
                    sub->timeout = timeout_usec;
                    sub->timeout_remaining = timeout_usec;
                    break;
                }
            });
        }
    });
}

bool fd_has_subscriptions() {
    FOR_EACH_FD_SUBJECT(FD_ALL, idx, set, {
        if (set->subscriptions) {
            return true;
        }
    });
    return false;
}

int fd_loop() {
    int ret = 0;

    active = true;

    int max_fd = -1;
    long long min_timeout = LLONG_MAX;
    fd_set *sets[3] = {};

    FOR_EACH_FD_SUBJECT(FD_ALL, idx, set, {
        if (set->subscriptions) {
            sets[idx] = &set->fds;
            FD_ZERO(&set->fds);
            vec_each(set->subscriptions, i, sub, {
                if (sub.fd == -1) {
                    ERROR("invalid fd");
                }
                if (sub.fd > max_fd) {
                    max_fd = sub.fd;
                }
                if (sub.timeout >= 0) {
                    if (sub.timeout_remaining < min_timeout) {
                        min_timeout = sub.timeout_remaining;
                    }
                }
                ASSERT(sub.fd >= 0);
                FD_SET(sub.fd, &set->fds);
            });
        }
    });

    if (max_fd == -1) {
        ERROR("no fds");
    }

    long long now;
    struct timeval tv;
    struct timeval *timeout = NULL;
    if (min_timeout != LLONG_MAX) {
        tv = usec_to_timeval(min_timeout > 0 ? min_timeout : 0);
        timeout = &tv;
        now = usec_timestamp();
    }

    ASSERT(select(max_fd + 1, sets[1], sets[2], sets[0], timeout) >= 0);

    long long elapsed = 0;
    if (min_timeout != LLONG_MAX) {
        elapsed = usec_timestamp() - now;
    }

    fd_event event;

    FOR_EACH_FD_SUBJECT(FD_ALL, idx, set, {
        if (set->subscriptions) {
            vec_each_ptr(set->subscriptions, i, sub, {
                if (sub->fd == -1) {
                    continue;
                }
                bool has_data = FD_ISSET(sub->fd, &set->fds);
                bool timed_out = !has_data
                    && sub->timeout_remaining == min_timeout;
                if (has_data || timed_out) {
                    sub->timeout_remaining = sub->timeout;
                    event.type = set->type;
                    event.fd = sub->fd;
                    event.timed_out = timed_out;
                    event.data = sub->data;
                    ASSERT(!sub->callback(event));
                }
                else {
                    sub->timeout_remaining -= elapsed;
                }
            });
            vec_each_r(set->subscriptions, i, sub, {
                if (sub.fd == -1) {
                    vec_remove_at(set->subscriptions, i);
                }
            });
            if (!vec_len(set->subscriptions)) {
                vec_free(set->subscriptions);
                set->subscriptions = NULL;
            }
        }
    });

exit:
    active = false;
    return ret;
}

