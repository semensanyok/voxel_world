#include "logging.h"
#include <cstdarg>
#include <cstdio>

void log(int severity, const char *function,
         const char *file, int line, const char *fmt, int argc, ...) {
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
  vfprintf(stderr, fmt, argv);
  va_end(argv);
  fprintf(stderr, fmt_loc, function, file, line);
};
void FLOG_ERROR(const char *fmt, const int argc...) {
  std::va_list args;
  va_start(args, argc);
  log(SEVERITY_LEVEL::ERROR, __FUNCTION__, __FILE__, __LINE__, fmt, argc, args);
  va_end(args);
}

