#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include "color_16.h"
#include "ansi.h"
#include "border.h"
#include "inline_term.h"

typedef struct {
    BorderType border;
    AnsiStyle border_style;
    bool capture_stdout;
    bool capture_stderr;
    char **command;
    InlineTermResizeConfig vertical_resize_config;
    InlineTermResizeConfig horizontal_resize_config;
} Config;

void parse_options(int argc, char *argv[], Config *config);

#endif
