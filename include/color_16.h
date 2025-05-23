#ifndef COLOR_16_H
#define COLOR_16_H

typedef enum {
    COLOR_16_UNKNOWN = -1,

    COLOR_16_BLACK,
    COLOR_16_RED,
    COLOR_16_GREEN,
    COLOR_16_YELLOW,
    COLOR_16_BLUE,
    COLOR_16_MAGENTA,
    COLOR_16_CYAN,
    COLOR_16_WHITE,

    COLOR_16_BRIGHT_BLACK,
    COLOR_16_BRIGHT_RED,
    COLOR_16_BRIGHT_GREEN,
    COLOR_16_BRIGHT_YELLOW,
    COLOR_16_BRIGHT_BLUE,
    COLOR_16_BRIGHT_MAGENTA,
    COLOR_16_BRIGHT_CYAN,
    COLOR_16_BRIGHT_WHITE,
} Color16;

Color16 color_16_parse(const char *name);

#endif
