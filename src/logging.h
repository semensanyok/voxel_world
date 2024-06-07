#ifndef VW_LOGGING
#define VW_LOGGING

enum SEVERITY_LEVEL {
        DEBUG,
        INFO,
        ERROR
};

#define FLOG_ERROR(fmt, argc, ...) \
FLOG(ERROR, fmt, argc, __VA_ARGS__);

#define LOG_ERROR(fmt) \
LOG(ERROR,  fmt);

#define FLOG_INFO(fmt, argc, ...) \
FLOG(INFO, fmt, argc, __VA_ARGS__);

#define LOG_INFO(fmt) \
LOG(INFO,  fmt);

#define FLOG_DEBUG(fmt, argc, ...) \
FLOG(DEBUG, fmt, argc, __VA_ARGS__);

#define LOG_DEBUG(fmt) \
LOG(DEBUG,  fmt);

#define LOG(severity, fmt) \
FLOG(severity, fmt, 0);\

#define FLOG(severity, fmt, argc, ...) \
log(severity,  __FUNCTION__, __FILE__, __LINE__, fmt, argc, __VA_ARGS__);

void log(int severity, const char *function,
         const char *file, int line, const char *fmt, int argc, ...);


#endif
