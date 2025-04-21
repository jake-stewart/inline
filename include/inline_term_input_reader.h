#ifndef INPUT_READER_H
#define INPUT_READER_H

#include <termkey.h>
#include <termios.h>
#include <vterm.h>

typedef struct {
    TermKey *tk;
    VTerm *vt;
    int fd;
    int mouse_col_offset;
    int mouse_row_offset;
    int mouse_button_pressed;
    int mouse_mod_pressed;
    struct termios orig_termios;
} InlineTermInputReader;

InlineTermInputReader *input_reader_new(VTerm *vt, int fd);
void input_reader_set_mouse_offset(
    InlineTermInputReader *reader,
    int row,
    int col
);
void input_reader_start(InlineTermInputReader *reader);
void input_reader_stop(InlineTermInputReader *reader);
void input_reader_free(InlineTermInputReader *reader);

#endif
