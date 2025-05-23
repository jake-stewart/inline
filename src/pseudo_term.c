#include "pseudo_term.h"
#include "error.h"

#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>

PseudoTerm *pseudo_term_new() {
    PseudoTerm *pty = malloc(sizeof(PseudoTerm));
    if (pty) {
        *pty = (PseudoTerm) {
            .master_fd = 0,
            .pid = 0,
            .fds = {-1, -1, -1},
            .alive = false,
            .exit_status = -1,
            .winsize = {},
            .initial_termios = NULL
        };
    }
    return pty;
}

void pseudo_term_free(PseudoTerm *pty) {
    if (pty->master_fd != -1) {
        close(pty->master_fd);
    }
    for (int i = 0; i < 3; i++) {
        if (pty->fds[i] != -1) {
            close(pty->fds[i]);
        }
    }
    free(pty);
}

int pseudo_term_kill(PseudoTerm *pty, int sig) {
    int ret = 0;
    ASSERT(!kill(pty->pid, sig));
exit:
    return ret;
}

int pseudo_term_resize(PseudoTerm *pty, int rows, int cols) {
    int ret = 0;
    if (rows != pty->winsize.ws_row || cols != pty->winsize.ws_row) {
        pty->winsize.ws_col = cols;
        pty->winsize.ws_row = rows;
        if (pty->alive) {
            ASSERT(!ioctl(pty->master_fd, TIOCSWINSZ, &pty->winsize));
            ASSERT(!kill(pty->pid, SIGWINCH));
        }
    }
exit:
    return ret;
}

int pseudo_term_start(PseudoTerm *pty, char **command, FdStrategy fd_strategy) {
    int ret = 0;
    pty->alive = false;

    ASSERT(!fd_strategy_prepare(&fd_strategy));

    pty->pid = forkpty(
        &pty->master_fd,
        NULL,
        pty->initial_termios,
        &pty->winsize
    );

    if (!pty->pid) {
        if (fd_strategy_apply_slave(&fd_strategy)) {
            _exit(1);
        }
        setenv("TERM", "xterm-256color", true);
        execvp(command[0], command);
        _exit(1);
    }
    else {
        ASSERT(pty->pid > 0);
        pty->alive = true;
        ASSERT(!fd_strategy_apply_master(&fd_strategy, pty->fds));
    }
exit:
    return ret;
}

int pseudo_term_read(
    PseudoTerm *term,
    char *buffer,
    size_t buf_size,
    size_t *bytes_read
) {
    int ret = 0;
    ASSERT(({
        ssize_t result = read(term->master_fd, buffer, sizeof buffer);
        if (result < 0 && errno == EIO) {
            result = 0;
        }
        if (result == 0) {
            term->alive = false;
        }
        *bytes_read = result;
        result >= 0;
    }));
exit:
    return ret;
}

int pseudo_term_exit_status(PseudoTerm *pty) {
    if (pty->exit_status != -1) {
        return pty->exit_status;
    }
    int status;
    waitpid(pty->pid, &status, 0);
    if (WIFEXITED(status)) {
        pty->exit_status = WEXITSTATUS(status);
    }
    else {
        pty->exit_status = 1;
    }
    return pty->exit_status;
}
