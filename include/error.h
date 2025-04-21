#include <string.h>
#include <errno.h>
#include <stdio.h>

extern _Thread_local const char *errmsg;
extern _Thread_local char errmsg_buffer[1024];

// #define DEBUG_ASSERT

#define ASSERT_NEW(expr) \
    if (!(expr)) { \
        goto cleanup; \
    }

#ifndef DEBUG_ASSERT

#define ERROR(...) \
    ret = -1; \
    snprintf(errmsg_buffer, 1024, __VA_ARGS__); \
    errmsg = errmsg_buffer; \
    goto exit; \

#define ASSERT_2(expr, message) { \
    errno = 0; \
    errmsg = NULL; \
    if (!(expr)) { \
        ret = errno ? errno : -1; \
        errmsg = message; \
        goto exit; \
    } \
}

#define ASSERT_1(expr) { \
    errno = 0; \
    errmsg = NULL; \
    if (!(expr)) { \
        ret = errno ? errno : -1; \
        goto exit; \
    } \
}

#define ASSERT_HELPER(_1, _2, NAME, ...) NAME
#define ASSERT(...) ASSERT_HELPER(__VA_ARGS__, ASSERT_2, ASSERT_1)(__VA_ARGS__)

#else

#define ERROR_IMPL(fmt, file, line, ...) \
    ret = -1; \
    snprintf(errmsg_buffer, 1024, "%s:%d: " fmt, \
             file, line, ##__VA_ARGS__); \
    errmsg = errmsg_buffer; \
    goto exit;

#define ERROR(fmt, ...) ERROR_IMPL(fmt, __FILE__, __LINE__, ##__VA_ARGS__)

#define ASSERT_2(expr, message, file, line) { \
    errno = 0; \
    errmsg = NULL; \
    if (!(expr)) { \
        ret = errno ? errno : -1; \
        snprintf(errmsg_buffer, 1024, "%s:%d: %s", \
                 file, line, message); \
        errmsg = errmsg_buffer; \
        goto exit; \
    } \
}

#define ASSERT_1(expr, file, line) { \
    errno = 0; \
    errmsg = NULL; \
    if (!(expr)) { \
        ret = errno ? errno : -1; \
        if (!errmsg) { \
            snprintf(errmsg_buffer, 1024, \
                     "%s:%d: unhandled error", file, line); \
            errmsg = errmsg_buffer; \
        } \
        goto exit; \
    } \
}

#define ASSERT_HELPER(_1, _2, NAME, ...) NAME
#define ASSERT(...) ASSERT_HELPER( \
    __VA_ARGS__, ASSERT_2, ASSERT_1)(__VA_ARGS__, __FILE__, __LINE__)

#endif

