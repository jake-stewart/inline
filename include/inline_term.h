#ifndef INLINE_TERM_H
#define INLINE_TERM_H

#include "rect.h"
#include "pseudo_term.h"
#include "inline_term_renderer.h"
#include "inline_term_input_reader.h"
#include "vec.h"
#include <vterm.h>

typedef struct {
    int mouse_mode;
    VTermPos cursor_pos;
    bool cursor_visible;
    AnsiCursorShape cursor_shape;
    bool cursor_blink;
} InlineTermState;

typedef struct {
    int exact;
    int min;
    int max;
    float percent;
} InlineTermResizeConfig;

typedef struct {
    PseudoTerm *pty;
    VTerm *vterm;
    InlineTermRenderer *renderer;
    InlineTermInputReader *reader;
    FILE *tty;
    FdStrategy fd_strategy;
    vec(char *) captured_output[3];
    size_t captured_output_buffer_remaining[3];
    BorderType border_type;
    AnsiStyle border_style;
    Rect rect;
    bool border;
    bool resized;
    InlineTermResizeConfig horizontal_resize_config;
    InlineTermResizeConfig vertical_resize_config;
    InlineTermState old_state;
    InlineTermState new_state;
    long long burst_start;
    long long last_output;
    long long render_timeout;
} InlineTerm;

InlineTerm *inline_term_new();
void inline_term_free(InlineTerm *term);

void inline_term_set_capture(InlineTerm *term, int fileno, bool enabled);
void inline_term_set_redirect_stdin(InlineTerm *term, bool enabled);

void inline_term_print_captured_output(InlineTerm *term, int fileno);

int inline_term_start(InlineTerm *term, char **command);

#endif
