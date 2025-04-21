#include "is_executable.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

int executable_status(const char *command) {
    int reason = ENOENT;
    const char *path;
    struct stat stats;
    char buffer[PATH_MAX];
    const char *end;
    const char *dir;
    ssize_t written;
    size_t len;

    if (strchr(command, '/')) {
        if (access(command, X_OK) == 0) {
            stat(command, &stats);
            if (S_ISDIR(stats.st_mode)) {
                return EISDIR;
            }
            return 0;
        }
        if (access(command, F_OK) == 0) {
            return EACCES;
        }
        return reason;
    }

    path = getenv("PATH");
    if (!path) {
        return reason;
    }
    do {
        end = path + strcspn(path, ":");
        len = end - path;
        dir = len ? path : ".";
        if (len == 0) {
            len = 1;
        }
        written = snprintf(buffer, sizeof(buffer),
            "%.*s/%s", (int)len, dir, command);
        
        if (written > 0 && written < sizeof(buffer)) {
            if (access(buffer, X_OK) == 0) {
                stat(command, &stats);
                if (S_ISDIR(stats.st_mode)) {
                    reason = EISDIR;
                }
                else {
                    return 0;
                }
            }
            else if (access(buffer, F_OK) == 0) {
                reason = EACCES;
            }
        }
        path = end + 1;
    }
    while (*end);
    return reason;
}

bool is_executable(const char *command) {
    int status = executable_status(command);
    if (status) {
        errno = status;
        return false;
    }
    return true;
}
