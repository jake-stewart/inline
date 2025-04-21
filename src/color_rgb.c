#include "color_rgb.h"
#include <string.h>
#include <stdio.h>

int color_rgb_parse(const char *str, ColorRgb *rgb) {
    int result = 0;

    if (str[0] == '#') {
        str++;
    }
    int len = strlen(str);

    if (len == 3) {
        result = sscanf(
            str,
            "%01hhx%01hhx%01hhx",
            &rgb->r,
            &rgb->g,
            &rgb->b
        );
        rgb->r = (rgb->r << 4) + rgb->r;
        rgb->g = (rgb->g << 4) + rgb->g;
        rgb->b = (rgb->b << 4) + rgb->b;
    }
    else if (len == 6) {
        result = sscanf(
            str,
            "%02hhx%02hhx%02hhx",
            &rgb->r,
            &rgb->g,
            &rgb->b
        );
    }

    return result == 3 ? 0 : -1;
}
