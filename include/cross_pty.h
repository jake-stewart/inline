#if defined(__FreeBSD__)
#include <libutil.h>
#include <sys/ioctl.h>
#include <termios.h>
#elif defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
#include <sys/ioctl.h>
#include <util.h>
#else
#include <pty.h>
#endif
