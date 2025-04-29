#include "inline_term.h"
#include "error.h"
#include "inline_term_renderer.h"
#include "fd.h"
#include "usec.h"
#include "vec.h"
#include "utils.h"
#include "signal_handler.h"
#include <unistd.h>
#include <signal.h>

InlineTerm *inline_term_new() {
    InlineTerm *term;
    ASSERT_NEW(term = malloc(sizeof(InlineTerm)));
    *term = (InlineTerm){};
    ASSERT_NEW(term->pty = pseudo_term_new());
    return term;

cleanup:
    if (term) {
        pseudo_term_free(term->pty);
        inline_term_renderer_free(term->renderer);
        free(term);
    }
    return NULL;
}

void inline_term_free(InlineTerm *term) {
    if (!term) {
        return;
    }
    if (term->reader) {
        input_reader_free(term->reader);
    }
    if (term->renderer) {
        inline_term_renderer_free(term->renderer);
    }
    if (term->vterm) {
        vterm_free(term->vterm);
    }
    if (term->pty) {
        pseudo_term_free(term->pty);
    }
    for (int i = STDOUT_FILENO; i <= STDERR_FILENO; i++) {
        if (term->captured_output[i]) {
            vec_free(term->captured_output[i]);
        }
    }
    free(term);
}

int damage(VTermRect rect, void *data) {
    InlineTerm *term = data;
    inline_term_renderer_damage(term->renderer, rect);
    return true;
}

int movecursor(VTermPos new_pos, VTermPos old_pos, int visible, void *data) {
    InlineTerm *term = data;
    term->new_state.cursor_visible = visible;
    term->new_state.cursor_pos = new_pos;
    return true;
}

int settermprop(VTermProp p, VTermValue *value, void *data) {
    bool ret;
    InlineTerm *term = data;
    if (p == VTERM_PROP_MOUSE) {
        term->new_state.mouse_mode = value->number;
        ret = true;
    }
    else if (p == VTERM_PROP_CURSORVISIBLE) {
        term->new_state.cursor_visible = value->boolean;
        ret = true;
    }
    else if (p == VTERM_PROP_CURSORSHAPE) {
        switch (value->number) {
            case VTERM_PROP_CURSORSHAPE_BAR_LEFT:
                term->new_state.cursor_shape = ANSI_CURSOR_SHAPE_BEAM;
                break;
            case VTERM_PROP_CURSORSHAPE_BLOCK:
                term->new_state.cursor_shape = ANSI_CURSOR_SHAPE_BLOCK;
                break;
            case VTERM_PROP_CURSORSHAPE_UNDERLINE:
                term->new_state.cursor_shape = ANSI_CURSOR_SHAPE_UNDERLINE;
                break;
        }
        ret = true;
    }
    else if (p == VTERM_PROP_CURSORBLINK) {
        term->new_state.cursor_blink = value->boolean;
        ret = true;
    }
    else {
        ret = false;
    }
    return ret;
}

void inline_term_set_capture(InlineTerm *term, int fileno, bool enabled) {
    term->fd_strategy.strategy[fileno] = enabled
        ? FD_STRATEGY_PIPE : FD_STRATEGY_NONE;
    if (enabled) {
        term->fd_strategy.strategy[fileno] = FD_STRATEGY_PIPE;
        if (!term->captured_output[fileno]) {
            term->captured_output[fileno] = vec_new();
        }
    }
    else {
        term->fd_strategy.strategy[fileno] = FD_STRATEGY_NONE;
        if (term->captured_output[fileno]) {
            vec_free(term->captured_output[fileno]);
            term->captured_output[fileno] = NULL;
        }
    }
}

void inline_term_print_captured_output(InlineTerm *term, int fileno) {
    if (term->captured_output[fileno]) {
        size_t len = vec_len(term->captured_output[fileno]);
        vec_each(term->captured_output[fileno], i, buf, {
            write(fileno, buf, i == len - 1
                  ? 4096 - term->captured_output_buffer_remaining[fileno]
                  : 4096);
        });
    }
}

void inline_term_set_redirect_stdin(InlineTerm *term, bool enabled) {
    term->fd_strategy.strategy[STDIN_FILENO] = enabled
        ? FD_STRATEGY_REDIRECT : FD_STRATEGY_NONE;
}

bool inline_term_state_update(InlineTerm *term, bool force) {
    InlineTermState old = term->old_state;
    InlineTermState new = term->new_state;
    bool different = false;
    if (force || old.mouse_mode != new.mouse_mode) {
        ansi_write("\x1b[?1002%c",
                   new.mouse_mode ? 'h' : 'l');
        different = true;
    }
    if (force || vterm_pos_cmp(old.cursor_pos, new.cursor_pos)) {
        different = true;
    }
    if (force || old.cursor_visible != new.cursor_visible) {
        ansi_set_cursor_visible(new.cursor_visible);
        different = true;
    }
    if (force || old.cursor_shape != new.cursor_shape
        || old.cursor_blink != new.cursor_blink
    ) {
        ansi_set_cursor_shape_blink(
            new.cursor_shape, new.cursor_blink);
        different = true;
    }
    term->old_state = term->new_state;
    return different;
}

void on_vterm_output(const char *s, size_t len, void *data) {
    InlineTerm *term = data;
    write(term->pty->master_fd, s, len);
}

void inline_term_update(InlineTerm *term, bool force) {
    vterm_screen_flush_damage(
        vterm_obtain_screen(term->vterm));
    bool updated = false;
    updated |= inline_term_renderer_render(term->renderer);
    updated |= inline_term_state_update(term, force);
    if (updated) {
        ansi_move(
            term->rect.y + term->new_state.cursor_pos.row,
            term->rect.x + term->new_state.cursor_pos.col
        );
        ansi_flush();
    }
}

int on_process_output(fd_event ev) {
    int ret = 0;
    InlineTerm *term = ev.data;
    if (ev.timed_out) {
        fd_clear_timeout(FD_READ, ev.fd);
        term->render_timeout = 0;
        inline_term_update(term, false);
    }
    else {
        char buffer[4096];
        size_t bytes_read;
        ASSERT(!pseudo_term_read(
            term->pty,
            buffer,
            sizeof buffer,
            &bytes_read
        ), "failed to read from process");
        if (bytes_read == 0) {
            fd_unsubscribe(FD_READ, ev.fd);
            input_reader_stop(term->reader);
            goto exit;
        }
        vterm_input_write(term->vterm, buffer, bytes_read);
        long long now = usec_timestamp();
        if (now - term->last_output < 1000) {
            if (!term->burst_start) {
                term->burst_start = now;
            }
        }
        else {
            term->burst_start = 0;
        }
        term->last_output = now;
        const int throttle = 32 * 1000;
        if (!term->burst_start
            || now - term->burst_start < throttle
        ) {
            inline_term_update(term, false);
            if (term->render_timeout) {
                term->render_timeout = 0;
                fd_clear_timeout(FD_READ, ev.fd);
            }
        }
        else if (term->render_timeout) {
            if (now >= term->render_timeout) {
                inline_term_update(term, false);
                term->render_timeout = now + throttle;
                fd_set_timeout(FD_READ, ev.fd, throttle);
            }
        }
        else {
            term->render_timeout = now + throttle;
            fd_set_timeout(FD_READ, ev.fd, throttle);
        }
    }
exit:
    if (ret) {
        fd_unsubscribe(FD_READ, ev.fd);
    }
    return ret;
}

int on_process_capture(fd_event ev) {
    int ret = 0;
    char *allocated = NULL;
    ASSERT(!ev.timed_out);
    InlineTerm *term = ev.data;
    bool is_stdout = ev.fd == term->pty->fds[STDOUT_FILENO];
    int fileno = is_stdout ? STDOUT_FILENO : STDERR_FILENO;
    vec(char*) output = term->captured_output[fileno];
    size_t remaining = term->captured_output_buffer_remaining[fileno];

    char *buffer;
    if (remaining == 0) {
        remaining = 4096;
        ASSERT(allocated = malloc(remaining));
        buffer = allocated;
    }
    else {
        buffer = vec_at(output, vec_len(output) - 1) + (4096 - remaining);
    }

    ssize_t bytes_read;
    ASSERT(({
        bytes_read = read(ev.fd, buffer, remaining);
    }) >= 0);
    if (bytes_read == 0) {
        fd_unsubscribe(FD_READ, ev.fd);
        goto exit;
    }

    if (allocated) {
        vec_push(output, allocated);
        term->captured_output_buffer_remaining[fileno] = 4096 - bytes_read;
    }
    else {
        term->captured_output_buffer_remaining[fileno] -= bytes_read;
    }

    struct termios termios;
    tcgetattr(term->pty->master_fd, &termios);
    char converted[8192];
    if (termios.c_oflag & ONLCR) {
        bytes_read = lf_to_crlf(buffer, bytes_read, converted);
        buffer = converted;
    }
    vterm_input_write(term->vterm, buffer, bytes_read);

exit:
    if (ret) {
        free(allocated);
        fd_unsubscribe(FD_READ, ev.fd);
    }
    return ret;
}

int inline_term_create_vterm(InlineTerm *term) {
    int ret = 0;

    ASSERT(term->vterm = vterm_new(term->rect.h, term->rect.w),
           "failed to create vterm");

    vterm_set_utf8(term->vterm, true);
    vterm_output_set_callback(term->vterm, on_vterm_output, term);

    VTermState *state = vterm_obtain_state(term->vterm);
    vterm_state_reset(state, true);

    static VTermStateCallbacks state_cbs = {};
    state_cbs.settermprop = settermprop;
    vterm_state_set_callbacks(state, &state_cbs, term);

    VTermScreen *screen = vterm_obtain_screen(term->vterm);
    vterm_screen_reset(screen, true);
    static VTermScreenCallbacks screen_cbs = {};
    screen_cbs.movecursor = movecursor;
    screen_cbs.damage = damage;
    screen_cbs.settermprop = settermprop;
    vterm_screen_set_callbacks(screen, &screen_cbs, term);
    vterm_screen_enable_altscreen(screen, true);
    // vterm_screen_enable_reflow(screen, true);
    // vterm_screen_set_damage_merge(screen, VTERM_DAMAGE_SCREEN);

exit:
    return ret;
}

void inline_term_calc_size(InlineTerm *term, struct winsize ws) {
    bool border = term->border_type != BORDER_TYPE_NONE;
    int size[2];
    int window_size[2] = { ws.ws_row, ws.ws_col };
    InlineTermResizeConfig configs[2] = {
        term->vertical_resize_config,
        term->horizontal_resize_config
    };
    arr_each(configs, i, config, {
        size[i] = config.exact;
        if (size[i] == 0) {
            if (config.percent) {
                size[i] = (window_size[i] * config.percent) - border * 2;
                if (config.max && size[i] >= config.max) {
                    size[i] = config.max;
                }
                if (config.min && size[i] <= config.min) {
                    size[i] = config.min;
                }
            }
            else {
                size[i] = ws.ws_col - border * 2;
            }
        }
        if (window_size[i] < size[i] + border * 2) {
            size[i] = window_size[i] - border * 2;
        }
        if (size[i] <= 0) {
            border = false;
            size[i] = window_size[i];
        }
    });
    term->border = border;
    term->rect.h = size[0];
    term->rect.w = size[1];
}

int inline_term_apply_size(InlineTerm *term, struct winsize ws) {
    int ret = 0;

    const int w = term->rect.w;
    const int h = term->rect.h;
    const bool border = term->border;

    const int cursor_row_offset =
        term->old_state.cursor_pos.row + term->border;

    int x, y;
    ASSERT(!ansi_get_pos(&y, &x));
    if (term->rect.y) {
        y -= cursor_row_offset;
    }

    const int box_h = h + border * 2;
    const int box_w = w + border * 2;
    const int remaining_screen_h = ws.ws_row - y;
    const int height_overflow = box_h - remaining_screen_h - 1;

    if (height_overflow > 0) {
        y -= height_overflow;
        ansi_scroll_up(height_overflow);
    }

    ansi_move(y, 1);
    ansi_reset_style();
    ansi_clear_til_eof();

    for (int i = 0; i < box_h - 1; i++) {
        ansi_write("\n");
    }
    ASSERT(!ansi_get_pos(&y, &x));

    term->rect.y = y - (box_h - 1) + border;
    term->rect.x = (ws.ws_col - box_w) / 2 + 1 + border;

    term->renderer->row = term->rect.y;
    term->renderer->col = term->rect.x;
    term->renderer->width = w;
    term->renderer->height = h;

    term->reader->mouse_col_offset = term->rect.x;
    term->reader->mouse_row_offset = term->rect.y;

    if (border) {
        inline_term_renderer_draw_border(
            term->renderer,
            term->border_type,
            term->border_style
        );
    }

exit:
    return ret;
}

int inline_term_resize(InlineTerm *term) {
    int ret = 0;
    struct winsize ws;
    ASSERT(!ioctl(STDERR_FILENO, TIOCGWINSZ, &ws));
    inline_term_calc_size(term, ws);
    ASSERT(!inline_term_apply_size(term, ws));
    vterm_set_size(term->vterm, term->rect.h, term->rect.w);
    if (term->pty->alive) {
        pseudo_term_resize(term->pty, term->rect.h, term->rect.w);
    }
    inline_term_update(term, false);
exit:
    return ret;
}

void on_resize_signal(int sig, void *data) {
    InlineTerm *term = data;
    term->resized = true;
}

int inline_term_prepare_mainloop(InlineTerm *term, char **command) {
    int ret = 0;
    struct winsize ws;
    ASSERT(!ioctl(STDERR_FILENO, TIOCGWINSZ, &ws));
    inline_term_calc_size(term, ws);
    ASSERT(!pseudo_term_resize(term->pty, term->rect.h, term->rect.w));
    ASSERT(!pseudo_term_start(term->pty, command, term->fd_strategy));
    ASSERT(term->tty = fopen("/dev/tty", "r+"), "failed to open /dev/tty");
    ansi_set_input_file(term->tty);
    ASSERT(!inline_term_create_vterm(term));
    ASSERT(term->reader = input_reader_new(term->vterm, fileno(term->tty)));
    ASSERT(term->renderer = inline_term_renderer_new(
        vterm_obtain_screen(term->vterm)));
    ASSERT(!inline_term_apply_size(term, ws));

exit:
    if (ret) {
        if (term->tty) {
            fclose(term->tty);
            term->tty = NULL;
        }
        if (term->vterm) {
            vterm_free(term->vterm);
            term->vterm = NULL;
        }
        if (term->reader) {
            input_reader_free(term->reader);
            term->reader = NULL;
        }
        if (term->pty) {
            pseudo_term_free(term->pty);
            term->pty = NULL;
        }
    }

    return ret;
}

int inline_term_mainloop(InlineTerm *term) {
    int ret = 0;

    term->new_state.mouse_mode = 0;
    term->new_state.cursor_visible = true;
    term->new_state.cursor_blink = true;
    term->new_state.cursor_shape = ANSI_CURSOR_SHAPE_BLOCK;
    term->new_state.cursor_pos.col = 0;
    term->new_state.cursor_pos.row = 0;
    inline_term_update(term, true);

    input_reader_start(term->reader);
    fd_subscribe(FD_READ, term->pty->master_fd,
             on_process_output, term);
    for (int i = STDOUT_FILENO; i <= STDERR_FILENO; i++) {
        if (term->pty->fds[i] != -1) {
            fd_subscribe(FD_READ, term->pty->fds[i],
                         on_process_capture, term);
        }
    }

    while (fd_has_subscriptions()) {
        ASSERT(({
            int status;
            if (term->resized) {
                status = inline_term_resize(term);
                if (!status) {
                    term->resized = false;
                }
            }
            else {
                status = fd_loop();
            }
            !status || status == EINTR && term->resized;
        }));
    }

exit:
    input_reader_stop(term->reader);
    fd_unsubscribe(FD_READ, term->pty->master_fd);
    for (int i = STDOUT_FILENO; i <= STDERR_FILENO; i++) {
        if (term->pty->fds[i] != -1) {
            fd_unsubscribe(FD_READ, term->pty->fds[i]);
        }
    }
    term->new_state.cursor_visible = true;
    term->new_state.cursor_shape = ANSI_CURSOR_SHAPE_BLOCK;
    term->new_state.mouse_mode = 0;
    inline_term_state_update(term, false);

    return ret;
}

void inline_term_stop(InlineTerm *term) {
    ansi_reset_style();
    if (term->rect.y > 0) {
        ansi_move(term->rect.y - term->border, 1);
    }
    ansi_clear_til_eof();
    ansi_flush();
}

int inline_term_start(InlineTerm *term, char **command) {
    int ret = 0;
    register_signal_handler(SIGWINCH, on_resize_signal, term);
    ASSERT(!inline_term_prepare_mainloop(term, command));
    ASSERT(!inline_term_mainloop(term));
exit:
    clear_signal_handler(SIGWINCH, on_resize_signal, term);
    inline_term_stop(term);
    return ret;
}
