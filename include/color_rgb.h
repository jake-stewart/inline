#ifndef COLOR_RGB_H
#define COLOR_RGB_H

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} color_rgb;

int color_rgb_parse(const char *str, color_rgb *rgb);

#endif
