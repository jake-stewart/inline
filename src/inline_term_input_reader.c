#include "inline_term_input_reader.h"
#include "fd.h"
#include "termkey.h"
#include "error.h"
#include <vterm.h>

VTermKey tk_keysym_to_vt(TermKeySym sym) {
    switch (sym) {
        case TERMKEY_SYM_BACKSPACE:
            return VTERM_KEY_BACKSPACE;
        case TERMKEY_SYM_TAB:
            return VTERM_KEY_TAB;
        case TERMKEY_SYM_ENTER:
            return VTERM_KEY_ENTER;
        case TERMKEY_SYM_ESCAPE:
            return VTERM_KEY_ESCAPE;

        // case TERMKEY_SYM_SPACE:
        case TERMKEY_SYM_DEL:
            return VTERM_KEY_BACKSPACE;

        case TERMKEY_SYM_UP:
            return VTERM_KEY_UP;
        case TERMKEY_SYM_DOWN:
            return VTERM_KEY_DOWN;
        case TERMKEY_SYM_LEFT:
            return VTERM_KEY_LEFT;
        case TERMKEY_SYM_RIGHT:
            return VTERM_KEY_RIGHT;
        // case TERMKEY_SYM_BEGIN:
        // case TERMKEY_SYM_FIND:
        case TERMKEY_SYM_INSERT:
            return VTERM_KEY_INS;
        case TERMKEY_SYM_DELETE:
            return VTERM_KEY_DEL;
        // case TERMKEY_SYM_SELECT:
        case TERMKEY_SYM_PAGEUP:
            return VTERM_KEY_PAGEUP;
        case TERMKEY_SYM_PAGEDOWN:
            return VTERM_KEY_PAGEDOWN;
        case TERMKEY_SYM_HOME:
            return VTERM_KEY_HOME;
        case TERMKEY_SYM_END:
            return VTERM_KEY_END;

        case TERMKEY_SYM_KP0:
            return VTERM_KEY_KP_0;
        case TERMKEY_SYM_KP1:
            return VTERM_KEY_KP_1;
        case TERMKEY_SYM_KP2:
            return VTERM_KEY_KP_2;
        case TERMKEY_SYM_KP3:
            return VTERM_KEY_KP_3;
        case TERMKEY_SYM_KP4:
            return VTERM_KEY_KP_4;
        case TERMKEY_SYM_KP5:
            return VTERM_KEY_KP_5;
        case TERMKEY_SYM_KP6:
            return VTERM_KEY_KP_6;
        case TERMKEY_SYM_KP7:
            return VTERM_KEY_KP_7;
        case TERMKEY_SYM_KP8:
            return VTERM_KEY_KP_8;
        case TERMKEY_SYM_KP9:
            return VTERM_KEY_KP_9;
        case TERMKEY_SYM_KPENTER:
            return VTERM_KEY_KP_ENTER;
        case TERMKEY_SYM_KPPLUS:
            return VTERM_KEY_KP_PLUS;
        case TERMKEY_SYM_KPMINUS:
            return VTERM_KEY_KP_MINUS;
        case TERMKEY_SYM_KPMULT:
            return VTERM_KEY_KP_MULT;
        case TERMKEY_SYM_KPDIV:
            return VTERM_KEY_KP_DIVIDE;
        case TERMKEY_SYM_KPCOMMA:
            return VTERM_KEY_KP_COMMA;
        case TERMKEY_SYM_KPPERIOD:
            return VTERM_KEY_KP_PERIOD;
        case TERMKEY_SYM_KPEQUALS:
            return VTERM_KEY_KP_EQUAL;

        default:
            return VTERM_KEY_NONE;
    }
}

int tk_mouse_button_to_vt(int button) {
    if (button >= 0 && button <= 6) {
        if (button == 0) {
            return 1;
        }
        return button;
    }
    return -1;
}

VTermModifier tk_modifier_to_vt(int modifiers) {
    VTermModifier mod = 0;
    if (modifiers & TERMKEY_KEYMOD_CTRL) {
        mod |= VTERM_MOD_CTRL;
    }
    if (modifiers & TERMKEY_KEYMOD_ALT) {
        mod |= VTERM_MOD_ALT;
    }
    if (modifiers & TERMKEY_KEYMOD_SHIFT) {
        mod |= VTERM_MOD_SHIFT;
    }
    return mod;
}

void input_reader_handle_keysym(
    InlineTermInputReader *reader,
    TermKeyKey key
) {
    VTermKey vt_key = tk_keysym_to_vt(key.code.sym);
    VTermModifier vt_mod = tk_modifier_to_vt(key.modifiers);
    if (vt_key != VTERM_KEY_NONE) {
        vterm_keyboard_key(reader->vt, vt_key, vt_mod);
    }
}

void input_reader_release_mouse(InlineTermInputReader *reader) {
    if (reader->mouse_button_pressed != -1) {
        vterm_mouse_button(
            reader->vt,
            reader->mouse_button_pressed,
            false,
            reader->mouse_mod_pressed
        );
        reader->mouse_button_pressed = -1;
    }
}

void input_reader_move_mouse(
    InlineTermInputReader *reader,
    int line,
    int col,
    int vt_mod
) {
    vterm_mouse_move(reader->vt, line, col, vt_mod);
}

void input_reader_press_mouse(
    InlineTermInputReader *reader,
    int line,
    int col,
    int vt_button,
    int vt_mod
) {
    input_reader_release_mouse(reader);
    input_reader_move_mouse(reader, line, col, VTERM_MOD_NONE);
    reader->mouse_button_pressed = vt_button;
    reader->mouse_mod_pressed = vt_mod;
    if (vt_button != -1) {
        vterm_mouse_button(
            reader->vt,
            vt_button,
            true,
            vt_mod
        );
    }
}

void input_reader_handle_mouse(InlineTermInputReader *reader, TermKeyKey key) {
    TermKeyMouseEvent mouse_event;
    int button, line, col;
    termkey_interpret_mouse(
        reader->tk, &key, &mouse_event, &button, &line, &col);
    int vt_button = tk_mouse_button_to_vt(button);
    VTermModifier vt_mod = tk_modifier_to_vt(key.modifiers);
    line -= reader->mouse_row_offset;
    col -= reader->mouse_col_offset;
    if (mouse_event == TERMKEY_MOUSE_DRAG) {
        input_reader_move_mouse(reader, line, col, vt_mod);
    }
    else if (mouse_event == TERMKEY_MOUSE_PRESS) {
        input_reader_press_mouse(reader, line, col, vt_button, vt_mod);
    }
    else if (mouse_event == TERMKEY_MOUSE_RELEASE) {
        input_reader_release_mouse(reader);
    }
}

void input_reader_handle_key(InlineTermInputReader *reader, TermKeyKey key) {
    VTermModifier vt_mod = 0;
    if (key.modifiers & TERMKEY_KEYMOD_CTRL) {
        vt_mod |= VTERM_MOD_CTRL;
    }
    if (key.modifiers & TERMKEY_KEYMOD_ALT) {
        vt_mod |= VTERM_MOD_ALT;
    }
    if (key.modifiers & TERMKEY_KEYMOD_SHIFT) {
        vt_mod |= VTERM_MOD_SHIFT;
    }
    if (key.type == TERMKEY_TYPE_UNICODE) {
        vterm_keyboard_unichar(reader->vt, key.code.codepoint, vt_mod);
    }
    else if (key.type == TERMKEY_TYPE_KEYSYM) {
        input_reader_handle_keysym(reader, key);
    }
    else if (key.type == TERMKEY_TYPE_MOUSE) {
        input_reader_handle_mouse(reader, key);
    }
}

int input_reader_fd_callback(fd_event ev) {
    int ret = 0;
    InlineTermInputReader *reader = ev.data;
    TermKeyKey key;
    TermKeyResult key_result;
    if (ev.timed_out) {
        key_result = termkey_getkey_force(reader->tk, &key);
        fd_clear_timeout(FD_READ, ev.fd);
    }
    else {
        char buffer[4096];
        ssize_t bytes_read;
        ASSERT(({
            bytes_read = read(ev.fd, buffer, sizeof buffer);
        }) > 0);
        termkey_push_bytes(reader->tk, buffer, bytes_read);
        key_result = termkey_getkey(reader->tk, &key);
    }
    do {
        switch (key_result) {
            case TERMKEY_RES_KEY:
                input_reader_handle_key(reader, key);
                key_result = termkey_getkey(reader->tk, &key);
                break;
            case TERMKEY_RES_AGAIN:
                fd_set_timeout(FD_READ, ev.fd, 1000);
                break;
            default:
                break;
        }
    }
    while (key_result == TERMKEY_RES_KEY);

exit:
    if (ret) {
        fd_unsubscribe(FD_READ, ev.fd);
    }
    return ret;
}

InlineTermInputReader *input_reader_new(VTerm *vt, int fd) {
    InlineTermInputReader *reader = malloc(sizeof(InlineTermInputReader));
    if (reader) {
        tcgetattr(0, &reader->orig_termios);

        reader->mouse_col_offset = 0;
        reader->mouse_row_offset = 0;
        reader->mouse_button_pressed = -1;
        reader->mouse_mod_pressed = 0;
        reader->vt = vt;
        reader->tk = termkey_new(fd, TERMKEY_FLAG_CTRLC | TERMKEY_FLAG_NOTERMIOS);
        if (!reader->tk) {
            free(reader);
            reader = NULL;
        }
        struct termios t = reader->orig_termios;
        t.c_lflag &= ECHOCTL; // stops newline when <c-c> before eof
        t.c_iflag &= ~ICRNL;  // differentiate newline and linefeed
        t.c_iflag &= ~IXON;   // allow ctrl+s ctrl+q keys
        t.c_lflag &= ISIG;    // generate exit signals
        t.c_lflag &= ~ECHO;   // disable echoing keys back to user
        t.c_lflag &= ~ICANON; // disable cannonical mode
        tcsetattr(0, TCSANOW, &t);
    }
    return reader;
}

void input_reader_set_mouse_offset(
    InlineTermInputReader *reader,
    int row,
    int col
) {
    reader->mouse_col_offset = col;
    reader->mouse_row_offset = row;
}

void input_reader_start(InlineTermInputReader *reader) {
    fd_subscribe(FD_READ, reader->fd, input_reader_fd_callback, reader);
}

void input_reader_stop(InlineTermInputReader *reader) {
    fd_unsubscribe(FD_READ, reader->fd);
}

void input_reader_free(InlineTermInputReader *reader) {
    if (reader) {
        tcsetattr(0, TCSANOW, &reader->orig_termios);
        termkey_free(reader->tk);
        free(reader);
    }
}
