#include "logging.h"
void log(SEVERITY_LEVEL severity, const char *function, const char *file1,
         int line, const char *fmt, int argc, ...) {
  switch (severity) {
  case SEVERITY_LEVEL::DEBUG:
    fprintf(stderr, "%s: ", "DEBUG");
    break;
  case SEVERITY_LEVEL::INFO:
    fprintf(stderr, "%s: ", "INFO");
    break;
  case SEVERITY_LEVEL::ERROR:
    fprintf(stderr, "%s: ", "ERROR");
    break;
  default:
    break;
  }
  std::va_list args;
  va_start(args, argc);
  vfprintf(stderr, fmt, args);
  va_end(args);
  const char *fmt_loc = " at: %s (%s:%i)\n";
  fprintf(stderr, fmt_loc, function, file1, line);
};
