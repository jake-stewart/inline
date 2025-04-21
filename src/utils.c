#include "utils.h"
#include <unistd.h>
#include <string.h>
#include <ctype.h>

size_t lf_to_crlf(char* input, size_t input_len, char* output) {
    size_t j = 0;
    for (size_t i = 0; i < input_len; ++i) {
        if (input[i] == '\n') {
            output[j++] = '\r';
            output[j++] = '\n';
        } else {
            output[j++] = input[i];
        }
    }
    return j;
}

bool str_startswith(const char *str, const char *prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

bool str_endswith(const char *str, const char *postfix) {
    size_t str_len = strlen(str);
    size_t postfix_len = strlen(postfix);
    if (postfix_len > str_len) {
        return false;
    }
    return strcmp(str + str_len - postfix_len, postfix) == 0;
}

char *str_tolower(const char *str) {
    int len = strlen(str);
    char *copy = malloc(len + 1);
    for (int i = 0; i < len; i++) {
        copy[i] = tolower(str[i]);
    }
    copy[len] = 0;
    return copy;
}
