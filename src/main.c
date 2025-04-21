#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>

#include "error.h"
#include "config.h"
#include "inline_term.h"
#include "is_executable.h"

int main(int argc, char **argv) {
    int ret = 0;
    int exit_status = 0;

    Config config = { 0 };
    InlineTerm *term = NULL;

    parse_options(argc, argv, &config);
    ansi_set_output_file(config.capture_stdout ? stderr : stdout);

    ASSERT(is_executable(config.command[0]),
           config.command[0]);

    vterm_check_version(VTERM_VERSION_MAJOR, VTERM_VERSION_MINOR);
    setlocale(LC_CTYPE, NULL);

    ASSERT(term = inline_term_new());
    inline_term_set_capture(term, STDOUT_FILENO, config.capture_stdout);
    inline_term_set_capture(term, STDERR_FILENO, config.capture_stderr);
    inline_term_set_redirect_stdin(term, !isatty(STDIN_FILENO));

    term->vertical_resize_config = config.vertical_resize_config;
    term->horizontal_resize_config = config.horizontal_resize_config;
    term->border_type = config.border;
    term->border_style = config.border_style;

    ASSERT(!inline_term_start(term, config.command));
    exit_status = pseudo_term_exit_status(term->pty);

    for (int i = STDOUT_FILENO; i <= STDERR_FILENO; i++) {
        if (term->captured_output[i]) {
            vec_each(term->captured_output[i], j, buf, {
                write(i, buf.buffer, buf.size);
            });
        }
    }

exit:
    if (term) {
        inline_term_free(term);
    }
    if (ret) {
        if (errmsg) {
            if (ret >= 0) {
                fprintf(stderr, "%s: %s: %s\n",
                        *argv, errmsg, strerror(ret));
            }
            else {
                fprintf(stderr, "%s: %s\n", *argv, errmsg);
            }
        }
        else {
            fprintf(stderr, "%s: unhandled error\n", *argv);
        }
        return 1;
    }
    return exit_status;
}
