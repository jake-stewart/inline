#ifndef BORDER_TYPE_H
#define BORDER_TYPE_H

#include "rect.h"
#include "ansi.h"

typedef enum {
    BORDER_TYPE_NONE,
    BORDER_TYPE_PLAIN,
    BORDER_TYPE_ROUND,
    BORDER_TYPE_ASCII
} BorderType;

void draw_border(Rect rect, BorderType type, AnsiStyle style);

#endif
