#include "log.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

void log_info(const char * fmt, ...) {
  #ifdef LogEnabled
  static char buf[256];
  int fmt_len = 256 - 27 - strlen(fmt);
  va_list args;
  va_start(args, fmt);
  strncpy(buf, "\x1b[1m[C]\x1b[0m \x1b[96mInfo\x1b[0m: ", 27);
  strncpy(&(buf[27]), fmt, fmt_len > 0 ? fmt_len : 0);
  vprintf(buf, args);
  va_end(args);
  #endif
}

void log_warn(const char * fmt, ...) {
  #ifdef LogEnabled
  static char buf[256];
  int fmt_len = 256 - 27 - strlen(fmt);
  va_list args;
  va_start(args, fmt);
  strncpy(buf, "\x1b[1m[C]\x1b[0m \x1b[96mWarn\x1b[0m: ", 27);
  strncpy(&(buf[27]), fmt, fmt_len > 0 ? fmt_len : 0);
  vprintf(buf, args);
  va_end(args);
  #endif
}

void log_err(const char * fmt, ...) {
  #ifdef LogEnabled
  static char buf[256];
  int fmt_len = 256 - 28 - strlen(fmt);
  va_list args;
  va_start(args, fmt);
  strncpy(buf, "\x1b[1m[C]\x1b[0m \x1b[96mError\x1b[0m: ", 28);
  strncpy(&(buf[28]), fmt, fmt_len > 0 ? fmt_len : 0);
  vprintf(buf, args);
  va_end(args);
  #endif
}
