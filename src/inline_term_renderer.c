#include "inline_term_renderer.h"
#include "ansi.h"
#include "utf8.h"
#include "utils.h"

bool inline_term_renderer_render(InlineTermRenderer *renderer) {
    VTermRect rect = renderer->damage;
    if (rect.start_row == -1) {
        return false;
    }
    VTermScreen *screen = renderer->screen;
    bool hasColor = false;
    VTermColor currentFg;
    VTermColor currentBg;
    VTermScreenCellAttrs currentAttrs;
    renderer->damage = (VTermRect){ -1, -1, -1, -1 };

    for (int row = rect.start_row; row < rect.end_row; row++) {
        VTermScreenCell cell;
        int col = rect.start_col;
        if (col > 0) {
            vterm_screen_get_cell(screen, (VTermPos){row, col - 1}, &cell);
            if (cell.width > 1) {
                col -= 1;
            }
        }
        ansi_move(renderer->row + row, renderer->col + col);
        while (col < rect.end_col) {
            vterm_screen_get_cell(screen, (VTermPos){row, col}, &cell);

            if (!hasColor || !vterm_color_is_equal(&currentFg, &cell.fg) ||
                !vterm_color_is_equal(&currentBg, &cell.bg) ||
                currentAttrs.bold != cell.attrs.bold ||
                currentAttrs.reverse != cell.attrs.reverse)
            {
                AnsiStyle style = {};

                if (VTERM_COLOR_IS_INDEXED(&cell.fg)) {
                    style.fg.type = ANSI_COLOR_TYPE_256;
                    style.fg.data.color_256 = cell.fg.indexed.idx;
                }
                else if (VTERM_COLOR_IS_DEFAULT_FG(&cell.fg)) {
                    style.fg.type = ANSI_COLOR_TYPE_NONE;
                }
                else if (VTERM_COLOR_IS_RGB(&cell.fg)) {
                    style.fg.type = ANSI_COLOR_TYPE_RGB;
                    style.fg.data.color_rgb = (ColorRgb) {
                        cell.fg.rgb.red,
                        cell.fg.rgb.green,
                        cell.fg.rgb.blue
                    };
                }

                if (VTERM_COLOR_IS_INDEXED(&cell.bg)) {
                    style.bg.type = ANSI_COLOR_TYPE_256;
                    style.bg.data.color_256 = cell.bg.indexed.idx;
                }
                else if (VTERM_COLOR_IS_DEFAULT_BG(&cell.bg)) {
                    style.bg.type = ANSI_COLOR_TYPE_NONE;
                }
                else if (VTERM_COLOR_IS_RGB(&cell.bg)) {
                    style.bg.type = ANSI_COLOR_TYPE_RGB;
                    style.bg.data.color_rgb = (ColorRgb) {
                        cell.bg.rgb.red,
                        cell.bg.rgb.green,
                        cell.bg.rgb.blue
                    };
                }

                if (cell.attrs.reverse) {
                    style.standout = true;
                }
                if (cell.attrs.bold) {
                    style.bold = true;
                }
                if (cell.attrs.italic) {
                    style.italic = true;
                }
                if (cell.attrs.underline) {
                    style.underline = ANSI_UNDERLINE_NONE;
                }

                hasColor = true;
                currentAttrs = cell.attrs;
                currentFg = cell.fg;
                currentBg = cell.bg;
                ansi_set_style(style);
            }

            if (cell.chars[0] < 128) {
                ansi_write("%c", cell.chars[0] ? cell.chars[0] : ' ');
                col++;
            }
            else {
                char buf[5];
                int size = utf8_codepoint_to_bytes(cell.chars[0], buf);
                if (size == -1) {
                    ansi_write(" ");
                }
                else {
                    ansi_write("%.*s", size, buf);
                }
                col += cell.width;
            }
        }
    }
    return true;
}

InlineTermRenderer *inline_term_renderer_new(VTermScreen *screen) {
    InlineTermRenderer *renderer = malloc(sizeof(InlineTermRenderer));
    if (renderer) {
        *renderer = (InlineTermRenderer) {};
        renderer->screen = screen;
        renderer->damage = (VTermRect) {
            .start_row = -1,
            .end_row = -1,
            .start_col = -1,
            .end_col = -1
        };
    }
    return renderer;
}

void inline_term_renderer_free(InlineTermRenderer *renderer) {
    free(renderer);
}

void inline_term_renderer_draw_border(
    InlineTermRenderer *renderer,
    BorderType type,
    AnsiStyle style
) {
    Rect rect = {
        .y = renderer->row - 1,
        .x = renderer->col - 1,
        .w = renderer->width + 2,
        .h = renderer->height + 2
    };
    draw_border(rect, type, style);
}

int inline_term_renderer_damage(
    InlineTermRenderer *renderer,
    VTermRect rect
) {
    if (renderer->damage.start_row == -1) {
        renderer->damage = rect;
    }
    else {
        renderer->damage = (VTermRect) {
            .start_row = MIN(renderer->damage.start_row, rect.start_row),
            .end_row = MAX(renderer->damage.end_row, rect.end_row),
            .start_col = MIN(renderer->damage.start_col, rect.start_col),
            .end_col = MAX(renderer->damage.end_col, rect.end_col),
        };
    }
    return true;
}
