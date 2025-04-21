#include "signal_handler.h"
#include "vec.h"
#include <stdbool.h>

typedef struct {
    int sig;
    void (*callback)(int sig, void *data);
    void *userdata;
} SignalHandler;

vec(SignalHandler) handlers = NULL;

void main_handler(int sig) {
    vec_each(handlers, i, handler, {
        if (handler.sig == sig) {
            handler.callback(sig, handler.userdata);
        }
    })
}

int register_signal_handler(
    int sig,
    void (*callback)(int sig, void *data),
    void *userdata
) {
    if (!handlers) {
        handlers = vec_new();
    }
    if (!vec_find(handlers, i, handler, handler.sig == sig)) {
        signal(sig, main_handler);
    }
    vec_push(handlers, ((SignalHandler) {
        .sig = sig,
        .callback = callback,
        .userdata = userdata,
    }));

    return 0;
}

int clear_signal_handler(
    int sig,
    void (*callback)(int sig, void *data),
    void *userdata
) {
    if (!handlers) {
        return 0;
    }
    ssize_t idx = vec_find_index(handlers, i, handler, ({
        handler.sig == sig
            && handler.callback == callback
            && handler.userdata == userdata;
    }));
    if (idx >= 0) {
        vec_remove_at(handlers, idx);
        if (!vec_find(handlers, i, handler, handler.sig == sig)) {
            signal(sig, NULL);
        }
    }
    return 0;
}
