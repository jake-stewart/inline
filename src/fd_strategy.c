#include "fd_strategy.h"
#include "error.h"
#include <unistd.h>

int fd_strategy_prepare(FdStrategy *strategy) {
    int ret = 0;
    for (int i = 0; i < 3; i++) {
        switch (strategy->strategy[i]) {
            case FD_STRATEGY_NONE:
                break;
            case FD_STRATEGY_PIPE:
                ASSERT(pipe(strategy->fds[i]) == 0);
                break;
            case FD_STRATEGY_REDIRECT:
                ASSERT((strategy->fds[i][0] = dup(i)) >= 0);
                break;
        }
    }

exit:
    return ret;
}

int fd_strategy_apply_slave(FdStrategy *strategy) {
    int ret = 0;
    for (int i = 0; i < 3; i++) {
        switch (strategy->strategy[i]) {
            case FD_STRATEGY_NONE:
                break;
            case FD_STRATEGY_PIPE:
                ASSERT(close(strategy->fds[i][i == STDIN_FILENO]) == 0);
                ASSERT(dup2(strategy->fds[i][i != STDIN_FILENO], i) >= 0);
                break;
            case FD_STRATEGY_REDIRECT:
                ASSERT(dup2(strategy->fds[i][0], i) >= 0);
                ASSERT(close(strategy->fds[i][0]) == 0);
                break;
        }
    }
exit:
    return ret;
}

int fd_strategy_apply_master(FdStrategy *strategy, int *fds) {
    int ret = 0;
    for (int i = 0; i < 3; i++) {
        switch (strategy->strategy[i]) {
            case FD_STRATEGY_NONE:
                fds[i] = -1;
                break;
            case FD_STRATEGY_PIPE:
                ASSERT(close(strategy->fds[i][i != STDIN_FILENO]) == 0);
                fds[i] = strategy->fds[i][i == STDIN_FILENO];
                break;
            case FD_STRATEGY_REDIRECT:
                fds[i] = -1;
                ASSERT(close(strategy->fds[i][0]) == 0);
                ASSERT(close(i) == 0);
                break;
        }
    }
exit:
    return ret;
}
