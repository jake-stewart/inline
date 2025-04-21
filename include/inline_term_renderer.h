#ifndef RENDERER_H
#define RENDERER_H

#include <vterm.h>
#include "color_16.h"
#include "ansi.h"
#include "border.h"

typedef struct {
    VTermScreen *screen;
    int row;
    int col;
    int width;
    int height;
    VTermRect damage;
} InlineTermRenderer;

InlineTermRenderer *inline_term_renderer_new(VTermScreen *screen);
void inline_term_renderer_free(InlineTermRenderer *renderer);

void inline_term_renderer_draw_border(
    InlineTermRenderer *renderer,
    BorderType type,
    AnsiStyle style
);

int inline_term_renderer_damage(InlineTermRenderer *renderer, VTermRect rect);
bool inline_term_renderer_render(InlineTermRenderer *renderer);

#endif
