#ifndef ANSI_H
#define ANSI_H

#include <stdio.h>
#include <stdbool.h>
#include "color_16.h"
#include "color_rgb.h"

void ansi_set_output_file(FILE *file);
void ansi_set_input_file(FILE *file);
void ansi_write(const char *format, ...);
void ansi_flush();

void ansi_move(int y, int x);
void ansi_move_home();
void ansi_move_up(int amount);
void ansi_move_down(int amount);
void ansi_move_right(int amount);
void ansi_move_left(int amount);
void ansi_scroll_up(int amount);
void ansi_scroll_down(int amount);
void ansi_scroll_left(int amount);
void ansi_scroll_right(int amount);

void ansi_set_alternate_buffer(bool value);

void ansi_clear_style();
void ansi_clear_til_eol();
void ansi_clear_til_sof();
void ansi_clear_til_eof();
void ansi_clear_term();

typedef enum {
    ANSI_CURSOR_SHAPE_BLOCK = 2,
    ANSI_CURSOR_SHAPE_UNDERLINE = 4,
    ANSI_CURSOR_SHAPE_BEAM = 6
} AnsiCursorShape;

void ansi_set_cursor_shape_blink(AnsiCursorShape shape, bool blink);
void ansi_set_cursor_shape(AnsiCursorShape shape);
void ansi_set_cursor_blink(bool blink);
void ansi_set_cursor_visible(bool visible);
void ansi_save_cursor();
void ansi_restore_cursor();

int ansi_get_pos(int *x, int *y);
int ansi_get_background_color(color_rgb *color);

void ansi_set_scroll_region(int min_row, int max_row);
void ansi_reset_scroll_region();

typedef enum {
    NO_UNDERLINE = 0,
    LINE,
    DOUBLE,
    CURLY,
    DOTTED,
    DASHED
} AnsiUnderlineType;

typedef enum {
    ANSI_COLOR_TYPE_NONE = 0,
    ANSI_COLOR_TYPE_256,
    ANSI_COLOR_TYPE_RGB
} AnsiColorType;

typedef struct {
    AnsiColorType type;
    union {
        unsigned char color_256;
        color_rgb color_rgb;
    } data;
} AnsiColor;

typedef struct {
    AnsiColor fg;
    AnsiColor bg;
    bool blend;
    bool bold;
    bool italic;
    bool strikethrough;
    bool blink;
    bool standout;
    bool dim;
    AnsiUnderlineType underline;
} AnsiStyle;

void ansi_set_style(AnsiStyle style);
void ansi_reset_style();

#endif
