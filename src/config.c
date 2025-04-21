#include "config.h"
#include "error.h"
#include "utils.h"
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

const char *prog;

void usage() {
    printf("USAGE: %s {OPTIONS} {PROGRAM}\n", prog);
    printf("\n");
    printf("OPTIONS:\n");
    printf("  -e  --stderr      capture and dump all stdout after process exits.\n");
    printf("  -o  --stdout      capture and dump all stderr after process exits.\n");
    printf("  -R  --round       show a round border around the terminal.\n");
    printf("  -A  --ascii       show an ascii border around the terminal.\n");
    printf("  -C  --color       change the color of the border.\n");
    printf("                    can be a named color, color index, or RGB.\n");
    printf("  -B  --borderless  do not show a border aroubd the terminal.\n");
    printf("  -r  --rows        size of terminal in rows or 0 for max.\n");
    printf("                    can be provided as a percentage of screen height.\n");
    printf("  -c  --cols        size of terminal in cols or 0 for max.\n");
    printf("                    can be provided as a percentage of screen width.\n");
    printf("  -h  --help        show this message.\n");
    printf("  -v  --version     print version information.\n");
}

void color_arg(const char *name, AnsiColor *dest) {
    color_16 c16 = color_16_parse(optarg);
    if (c16 != COLOR_16_UNKNOWN) {
        dest->data.color_256 = c16;
        dest->type = ANSI_COLOR_TYPE_256;
        return;
    }
    int c256;
    int len = 0;
    if (
        sscanf(optarg, "%d%n", &c256, &len) == 1
        && len == strlen(optarg)
        && c256 < 256
        && c256 >= 0
    ) {
        dest->data.color_256 = c256;
        dest->type = ANSI_COLOR_TYPE_256;
        return;
    }
    color_rgb rgb;
    if (color_rgb_parse(optarg, &rgb) == 0) {
        dest->data.color_rgb = rgb;
        dest->type = ANSI_COLOR_TYPE_RGB;
        return;
    }

    fprintf(stderr, "%s: invalid color for %s: '%s'\n",
            prog, name, optarg);
    exit(2);
}

void int_arg(
    const char *name,
    int *dest,
    int min,
    int max
) {
    int len = 0;
    if (sscanf(optarg, "%d%n", dest, &len) != 1 || len != strlen(optarg)) {
        fprintf(stderr, "%s: invalid integer for %s: '%s'\n",
                prog, name, optarg);
        exit(2);
    }
    if (*dest < min) {
        fprintf(stderr, "%s: %s cannot be under %d\n",
                prog, name, min);
        exit(2);
    }
    if (*dest > max) {
        fprintf(stderr, "%s: %s cannot be over %d\n",
                prog, name, max);
        exit(2);
    }
}

void percent_arg(
    const char *name,
    float *dest,
    float min,
    float max
) {
    int len = 0;
    if (sscanf(optarg, "%f%%%n", dest, &len) != 1 || len != strlen(optarg)) {
        fprintf(stderr, "%s: invalid percent for %s: '%s'\n",
                prog, name, optarg);
        exit(2);
    }
    *dest /= 100.0;
    if (*dest < min) {
        fprintf(stderr, "%s: %s cannot be under %.0f%%\n",
                prog, name, min * 100);
        exit(2);
    }
    if (*dest > max) {
        fprintf(stderr, "%s: %s cannot be over %.0f%%\n",
                prog, name, max * 100);
        exit(2);
    }
}

void parse_options(int argc, char *argv[], Config *config) {
    prog = argv[0];

    AnsiStyle border_style = { 0 };
    border_style.fg.type = ANSI_COLOR_TYPE_256;
    border_style.fg.data.color_256 = COLOR_16_BRIGHT_BLACK;

    *config = (Config) { 0 };
    config->border = BORDER_TYPE_PLAIN;
    config->border_style = border_style;
    config->vertical_resize_config.exact = 10;

    struct option options[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"borderless", no_argument, 0, 'B'},
        {"round", no_argument, 0, 'R'},
        {"ascii", no_argument, 0, 'A'},
        {"color", required_argument, 0, 'C'},
        {"rows", required_argument, 0, 'r'},
        {"cols", required_argument, 0, 'c'},
        {"stdout", no_argument, 0, 'o'},
        {"stderr", no_argument, 0, 'e'},
        {0, 0, 0, 0}
    };

    int idx = 0;
    optind = 1;

    while (optind < argc && argv[optind][0] == '-') {
        int opt = getopt_long(
            argc,
            argv,
            "h v V B R A C: r: c: o e",
            options,
            &idx
        );
        switch (opt) {
            case 'o':
                config->capture_stdout = true;
                break;
            case 'e':
                config->capture_stderr = true;
                break;
            case 'r':
                if (str_endswith(optarg, "%")) {
                    config->vertical_resize_config.exact = 0;
                    percent_arg("rows",
                        &config->vertical_resize_config.percent, 0, 1);
                }
                else {
                    config->vertical_resize_config.percent = 0;
                    int_arg("rows",
                        &config->vertical_resize_config.exact, 0, USHRT_MAX);
                }
                break;
            case 'c':
                if (str_endswith(optarg, "%")) {
                    config->horizontal_resize_config.exact = 0;
                    percent_arg("cols",
                        &config->horizontal_resize_config.percent, 0, 1);
                }
                else {
                    config->horizontal_resize_config.percent = 0;
                    int_arg("cols",
                        &config->horizontal_resize_config.exact, 0, USHRT_MAX);
                }
                break;
            case 'B':
                config->border = BORDER_TYPE_NONE;
                break;
            case 'R':
                config->border = BORDER_TYPE_ROUND;
                break;
            case 'A':
                config->border = BORDER_TYPE_ASCII;
                break;
            case 'C':
                if (config->border == BORDER_TYPE_NONE) {
                    config->border = BORDER_TYPE_PLAIN;
                }
                color_arg("border", &config->border_style.fg);
                break;
            case 'v':
            case 'V':
                printf("inline 1.0.0\n");
                exit(0);
                break;
            case 'h':
                usage();
                exit(0);
            default:
                exit(2);
        }
    }

    if (argc > optind) {
        config->command = &argv[optind];
    }
    else {
        static char *default_command[2];
        char *shell = getenv("SHELL");
        if (shell == NULL) {
            shell = "/bin/sh";
        }
        default_command[0] = shell;
        default_command[1] = NULL;
        config->command = default_command;
    }
}
