#ifndef UTF8_H
#define UTF8_H

#include <stdlib.h>
#include <stdint.h>

int utf8_glyph_len(unsigned char ch);
int utf8_codepoint_to_bytes(uint32_t cp, char *buf);
int utf8_bytes_to_codepoint(const char *buf, uint32_t *cp);

#endif
