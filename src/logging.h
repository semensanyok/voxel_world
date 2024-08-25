#ifndef VW_LOGGING
#define VW_LOGGING

#include <cstdarg>
#include <cstdio>

enum class SEVERITY_LEVEL { DEBUG, INFO, ERROR };

void log(SEVERITY_LEVEL severity, const char *function, const char *file1,
         int line, const char *fmt, int argc, ...);

#define FLOG_ERROR(fmt, argc, ...)                                             \
  FLOG(SEVERITY_LEVEL::ERROR, fmt, argc, __VA_ARGS__);

#define LOG_ERROR(fmt) LOG(SEVERITY_LEVEL::ERROR, fmt);

#define FLOG_INFO(fmt, argc, ...)                                              \
  FLOG(SEVERITY_LEVEL::INFO, fmt, argc, __VA_ARGS__);

#define LOG_INFO(fmt) LOG(SEVERITY_LEVEL::INFO, fmt);

#define FLOG_DEBUG(fmt, argc, ...)                                             \
  FLOG(SEVERITY_LEVEL::DEBUG, fmt, argc, __VA_ARGS__);

#define LOG_DEBUG(fmt) LOG(SEVERITY_LEVEL::DEBUG, fmt);

#define LOG(severity, fmt) FLOG(severity, fmt, 0);

#define FLOG(severity, fmt, argc, ...)                                         \
  log(severity, __FUNCTION__, __FILE__, __LINE__, fmt, argc, __VA_ARGS__);

#endif
