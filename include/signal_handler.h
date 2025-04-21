#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

int register_signal_handler(
    int sig,
    void (*callback)(int signal, void *data),
    void *userdata
);

int clear_signal_handler(
    int sig,
    void (*callback)(int signal, void *data),
    void *userdata
);

#endif
