#include "fd_strategy.h"
#include <unistd.h>

void fd_strategy_prepare(FdStrategy *strategy) {
    for (int i = 0; i < 3; i++) {
        switch (strategy->strategy[i]) {
            case FD_STRATEGY_NONE:
                break;
            case FD_STRATEGY_PIPE:
                pipe(strategy->fds[i]);
                break;
            case FD_STRATEGY_REDIRECT:
                strategy->fds[i][0] = dup(i);
                break;
        }
    }
}

void fd_strategy_apply_slave(FdStrategy *strategy) {
    for (int i = 0; i < 3; i++) {
        switch (strategy->strategy[i]) {
            case FD_STRATEGY_NONE:
                break;
            case FD_STRATEGY_PIPE:
                close(strategy->fds[i][i == STDIN_FILENO]);
                dup2(strategy->fds[i][i != STDIN_FILENO], i);
                break;
            case FD_STRATEGY_REDIRECT:
                dup2(strategy->fds[i][0], i);
                close(strategy->fds[i][0]);
                break;
        }
    }
}

void fd_strategy_apply_master(FdStrategy *strategy, int *fds) {
    for (int i = 0; i < 3; i++) {
        switch (strategy->strategy[i]) {
            case FD_STRATEGY_NONE:
                fds[i] = -1;
                break;
            case FD_STRATEGY_PIPE:
                close(strategy->fds[i][i != STDIN_FILENO]);
                fds[i] = strategy->fds[i][i == STDIN_FILENO];
                break;
            case FD_STRATEGY_REDIRECT:
                fds[i] = -1;
                close(strategy->fds[i][0]);
                close(i);
                break;
        }
    }
}
