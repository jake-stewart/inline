#include "error.h"

_Thread_local const char *errmsg = NULL;
_Thread_local char errmsg_buffer[1024];
