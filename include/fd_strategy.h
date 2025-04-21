#ifndef FD_STRATEGY_H
#define FD_STRATEGY_H

typedef enum {
    FD_STRATEGY_NONE,
    FD_STRATEGY_PIPE,
    FD_STRATEGY_REDIRECT,
} FdStrategyType;

typedef struct {
    int fds[3][2];
    FdStrategyType strategy[3];
} FdStrategy;

int fd_strategy_prepare(FdStrategy *strategy);
int fd_strategy_apply_slave(FdStrategy *strategy);
int fd_strategy_apply_master(FdStrategy *strategy, int *fds);

#endif
