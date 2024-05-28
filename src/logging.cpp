#include "logging.h"
#include <cstdarg>
#include <cstdio>

void log(SEVERITY_LEVEL severity, const char *function,
         const char *file, int line, const char *fmt, int argc, ...) {
  const char* level_str;
  switch (severity) {
  case DEBUG:
    fprintf(stderr, "%s: ", "DEBUG");
    break;
  case INFO:
    fprintf(stderr, "%s: ", "INFO");
    break;
  case ERROR:
    fprintf(stderr, "%s: ", "ERROR");
    break;
  default:
    break;
  }
  const char* fmt_loc = "at: %s (%s:%i)\n";
  va_list argv;
  va_start(argv, argc);
  fprintf(stderr, fmt, argv);
  va_end(argv);
  fprintf(stderr, fmt_loc, level_str, function, file, line);
};
