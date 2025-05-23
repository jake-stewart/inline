#include "color_16.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>

Color16 color_16_parse(const char *name) {
    char *lower_name = str_tolower(name);
    char *color_name = lower_name;

    bool bright = false;
    if (str_startswith(color_name, "bright")) {
        bright = true;
        color_name += 6;
    }
    else if (str_startswith(color_name, "br")) {
        bright = true;
        color_name += 2;
    }
    if (bright) {
        color_name += color_name[0] == '-'
            || color_name[0] == '_'
            || color_name[0] == ' ';
    }

    Color16 color = COLOR_16_UNKNOWN;

    if (!strcmp(color_name, "red")) {
        color = COLOR_16_RED;
    }
    else if (!strcmp(color_name, "green")) {
        color = COLOR_16_GREEN;
    }
    else if (!strcmp(color_name, "blue")) {
        color = COLOR_16_BLUE;
    }
    else if (!strcmp(color_name, "cyan")) {
        color = COLOR_16_CYAN;
    }
    else if (!strcmp(color_name, "yellow")) {
        color = COLOR_16_YELLOW;
    }
    else if (!strcmp(color_name, "magenta")
        || !strcmp(name, "purple")
    ) {
        color = COLOR_16_MAGENTA;
    }
    else if (!strcmp(color_name, "black")) {
        color = COLOR_16_BLACK;
    }
    else if (!strcmp(color_name, "white")) {
        color = COLOR_16_WHITE;
    }

    free(lower_name);
    if (color == COLOR_16_UNKNOWN) {
        return color;
    }
    return bright ? color + 8 : color;
}
