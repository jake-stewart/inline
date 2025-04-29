#include "ansi.h"
#include "error.h"
#include "timeout_read.h"
#include <stdarg.h>
#include <unistd.h>

#define ANSI_ESC "\x1b["

char output_buffer[100000];
FILE *output = NULL;
FILE *input = NULL;

void ansi_set_input_file(FILE *file) {
    input = file;
}

void ansi_set_output_file(FILE *file) {
    // avoid flickering
    setvbuf(file, output_buffer, _IOFBF, sizeof(output_buffer));
    output = file;
}

void ansi_write(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(output, format, args);
    va_end(args);
}

void ansi_flush() {
    fflush(output);
}

void ansi_move(int y, int x) {
    ansi_write(ANSI_ESC "%d;%dH", y, x);
}

void ansi_move_home() {
    ansi_write(ANSI_ESC "H");
}

#define MOVEMENT(name, c) \
    void name(int amount) { \
        if (amount == 1) { \
            ansi_write(ANSI_ESC c); \
        } \
        else if (amount > 0) { \
            ansi_write(ANSI_ESC "%d" c, amount); \
        } \
    }

MOVEMENT(ansi_move_up, "A");
MOVEMENT(ansi_move_down, "B");
MOVEMENT(ansi_move_right, "C");
MOVEMENT(ansi_move_left, "D");
MOVEMENT(ansi_scroll_up, "S");
MOVEMENT(ansi_scroll_down, "T");
MOVEMENT(ansi_scroll_left, "@");
MOVEMENT(ansi_scroll_right, "A");

void ansi_set_alternate_buffer(bool value) {
    ansi_write(ANSI_ESC "?1049%c", value ? 'h' : 'l');
}

void ansi_clear_style() {
    ansi_write(ANSI_ESC "0m");
}

void ansi_clear_til_eol() {
    ansi_write(ANSI_ESC "K");
}

void ansi_clear_til_sof() {
    ansi_write(ANSI_ESC "1J");
}

void ansi_clear_til_eof() {
    ansi_write(ANSI_ESC "0J");
}

void ansi_clear_term() {
    ansi_write(ANSI_ESC "2J");
}

void ansi_set_cursor_visible(bool value) {
    ansi_write(ANSI_ESC "?25%c", value ? 'h' : 'l');
}

void ansi_save_cursor() {
    ansi_write("\x1b" "7");
}

void ansi_restore_cursor() {
    ansi_write("\x1b" "8");
}

void ansi_set_scroll_region(int min_row, int max_row) {
    ansi_write(ANSI_ESC "%d;%dr", min_row, max_row);
}

void ansi_reset_scroll_region() {
    ansi_write(ANSI_ESC "r");
}

static AnsiCursorShape cursor_shape = ANSI_CURSOR_SHAPE_BLOCK;
static bool cursor_blink = true;

void ansi_set_cursor_shape_blink(AnsiCursorShape shape, bool blink) {
    cursor_shape = shape ? shape : ANSI_CURSOR_SHAPE_BLOCK;
    cursor_blink = blink;
    ansi_write("\x1b[%d q", shape - blink);
    ansi_flush();
}

void ansi_set_cursor_shape(AnsiCursorShape shape) {
    ansi_set_cursor_shape_blink(shape, cursor_blink);
}

void ansi_set_cursor_blink(bool blink) {
    ansi_set_cursor_shape_blink(cursor_shape, blink);
}

int ansi_get_pos(int *y, int *x) {
    int ret = 0;
    char buf[128];
    int len = 0;
    ansi_write("\033[6n");
    ansi_flush();
    do {
        ssize_t bytes_read;
        ASSERT(({
            bytes_read = timeout_read(
                fileno(input), buf + len, 1, 100000);
        }) > 0, "failed to read cursor position");
    }
    while (buf[len++] != 'R' && len < 127);
    buf[len] = 0;
    ASSERT(sscanf(buf, "\x1b[%d;%dR", y, x) == 2,
           "failed to parse cursor position");

    char buffer[1024];
    sscanf(buf, "%*s%d", (int*)NULL);

exit:
    return ret;
}

int ansi_get_background_color(ColorRgb *color) {
    int ret = 0;
    char buf[128];
    int len = 0;
    ansi_write("\x1b]11;?\x07");
    ansi_flush();
    do {
        ssize_t bytes_read;
        ASSERT(({
            bytes_read = timeout_read(
                fileno(input), buf + len, 1, 100000);
        }) > 0, "failed to read background color");
    }
    while (buf[len++] != '\x07' && len < 127);
    buf[len] = 0;
    int r, g, b;
    ASSERT(sscanf(buf, "\x1b]11;rgb:%x/%x/%x", &r, &g, &b) == 3,
           "failed to parse background color");
    color->r = r / 256;
    color->g = g / 256;
    color->b = b / 256;

exit:
    return ret;
}

void ansi_set_style(AnsiStyle style) {
    ansi_write(ANSI_ESC "0");

    switch (style.fg.type) {
        case ANSI_COLOR_TYPE_NONE:
            break;
        case ANSI_COLOR_TYPE_256:
            if (style.fg.data.color_256 < 8) {
                ansi_write(";%d", 30 + style.fg.data.color_256);
            }
            else {
                ansi_write(";38;5;%d", style.fg.data.color_256);
            }
            break;
        case ANSI_COLOR_TYPE_RGB:
            ansi_write(";38;2;%d;%d;%d",
                style.fg.data.color_rgb.r,
                style.fg.data.color_rgb.g,
                style.fg.data.color_rgb.b);
            break;
    }

    switch (style.bg.type) {
        case ANSI_COLOR_TYPE_NONE:
            break;
        case ANSI_COLOR_TYPE_256:
            if (style.bg.data.color_256 < 8) {
                ansi_write(";%d", 40 + style.bg.data.color_256);
            }
            else {
                ansi_write(";48;5;%d", style.bg.data.color_256);
            }
            break;
        case ANSI_COLOR_TYPE_RGB:
            ansi_write(";48;2;%d;%d;%d",
                style.bg.data.color_rgb.r,
                style.bg.data.color_rgb.g,
                style.bg.data.color_rgb.b);
            break;
    }

    if (style.bold) {
        ansi_write(";1");
    }
    if (style.dim) {
        ansi_write(";2");
    }
    if (style.italic) {
        ansi_write(";3");
    }
    if (style.blink) {
        ansi_write(";5");
    }
    if (style.standout) {
        ansi_write(";7");
    }
    if (style.strikethrough) {
        ansi_write(";9");
    }

    if (style.underline) {
        ansi_write(";4");
        switch (style.underline) {
            case ANSI_UNDERLINE_DOUBLE:
                ansi_write(":2");
                break;
            case ANSI_UNDERLINE_CURLY:
                ansi_write(":3");
                break;
            case ANSI_UNDERLINE_DOTTED:
                ansi_write(":4");
                break;
            case ANSI_UNDERLINE_DASHED:
                ansi_write(":5");
                break;
            default:
                break;
        }
    }

    ansi_write("m");
}

void ansi_reset_style() {
    ansi_write(ANSI_ESC "0m");
}
