#ifndef PSEUDO_TERM_H
#define PSEUDO_TERM_H

#include <stdbool.h>
#include <stddef.h>
#include "cross_pty.h"
#include "fd_strategy.h"

typedef struct {
    int exit_status;
    int master_fd;
    int fds[3];
    pid_t pid;
    struct winsize winsize;
    struct termios *initial_termios;
    bool alive;
} PseudoTerm;

PseudoTerm *pseudo_term_new();
void pseudo_term_free(PseudoTerm *term);
int pseudo_term_exit_status(PseudoTerm *term);
int pseudo_term_kill(PseudoTerm *term, int sig);
int pseudo_term_resize(PseudoTerm *term, int rows, int cols);
int pseudo_term_read(
    PseudoTerm *term,
    char *buffer,
    size_t buf_size,
    size_t *bytes_read
);
int pseudo_term_start(
    PseudoTerm *term,
    char **command,
    FdStrategy fd_strategy
);

#endif
