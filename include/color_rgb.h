#ifndef COLOR_RGB_H
#define COLOR_RGB_H

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} ColorRgb;

int color_rgb_parse(const char *str, ColorRgb *rgb);

#endif
