#include "border.h"
#include "ansi.h"

#define HORIZONTAL chars[0]
#define VERTICAL chars[1]
#define TOP_LEFT chars[2]
#define TOP_RIGHT chars[3]
#define BOTTOM_LEFT chars[4]
#define BOTTOM_RIGHT chars[5]

const char *BORDER_CHARS[][6] = {
    { " ", " ", " ", " ", " ", " " },
    { "─", "│", "┌", "┐", "└", "┘" },
    { "─", "│", "╭", "╮", "╰", "╯" },
    { "-", "|", "+", "+", "+", "+" },
};

void draw_border(Rect rect, BorderType type, AnsiStyle style) {
    const char **chars = BORDER_CHARS[type];

    ansi_move(rect.y, rect.x);
    ansi_set_style(style);
    ansi_write(TOP_LEFT);
    for (int x = 0; x < rect.w - 2; x++) {
        ansi_write(HORIZONTAL);
    }
    ansi_write(TOP_RIGHT);

    for (int y = 1; y < rect.h - 1; y++) {
        ansi_move(rect.y + y, rect.x);
        ansi_write(VERTICAL);
        ansi_move(rect.y + y, rect.x + rect.w - 1);
        ansi_write(VERTICAL);
    }

    ansi_move(rect.y + rect.h - 1, rect.x);
    ansi_write(BOTTOM_LEFT);
    for (int x = 0; x < rect.w - 2; x++) {
        ansi_write(HORIZONTAL);
    }
    ansi_write(BOTTOM_RIGHT);
}
