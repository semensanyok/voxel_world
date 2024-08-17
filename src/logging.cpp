#include "logging.h"
#include <cstdarg>
#include <cstdio>

void FLOG_ERROR(const char *fmt, const int argc, va_list args) {
  log(SEVERITY_LEVEL::ERROR, __FUNCTION__, __FILE__, __LINE__, fmt, argc, args);
  va_end(args);
}

void FLOG_INFO(const char *fmt, const int argc, va_list args) {
  log(SEVERITY_LEVEL::INFO, __FUNCTION__, __FILE__, __LINE__, fmt, argc, args);
  va_end(args);
}

void FLOG_DEBUG(const char *fmt, const int argc, va_list args) {
  log(SEVERITY_LEVEL::DEBUG, __FUNCTION__, __FILE__, __LINE__, fmt, argc, args);
  va_end(args);
}
void LOG_ERROR(const char *fmt) {
  log(SEVERITY_LEVEL::ERROR, __FUNCTION__, __FILE__, __LINE__, fmt, 0,
      va_list());
};

void LOG_INFO(const char *fmt) {
  log(SEVERITY_LEVEL::INFO, __FUNCTION__, __FILE__, __LINE__, fmt, 0,
      va_list());
};

void LOG_DEBUG(const char *fmt) {
  log(SEVERITY_LEVEL::DEBUG, __FUNCTION__, __FILE__, __LINE__, fmt, 0,
      va_list());
};
void FLOG_INFO(const char *fmt, const int argc, va_list args);
void LOG_INFO(const char *fmt);
void FLOG_DEBUG(const char *fmt, const int argc, va_list args);
void LOG_DEBUG(const char *fmt);
void log(int severity, const char *function, const char *file, int line,
         const char *fmt, int argc, va_list args) {
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
  const char *fmt_loc = "at: %s (%s:%i)\n";
  vfprintf(stderr, fmt, args);
  fprintf(stderr, fmt_loc, function, file, line);
};
