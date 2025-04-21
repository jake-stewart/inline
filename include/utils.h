#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdlib.h>

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

// output size assumed to be twice input size
size_t lf_to_crlf(char* input, size_t input_len, char* output);

bool str_startswith(const char *str, const char *prefix);
bool str_endswith(const char *str, const char *postfix);
char *str_tolower(const char *str);

#endif
