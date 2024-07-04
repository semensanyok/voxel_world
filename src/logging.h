#ifndef VW_LOGGING
#define VW_LOGGING

#include <cstdarg>
enum SEVERITY_LEVEL {
        DEBUG,
        INFO,
        ERROR
};
void log(int severity, const char *function,
         const char *file, int line, const char *fmt, int argc, va_list);
void FLOG_ERROR(const char *fmt, const int argc...);
void LOG_ERROR( const char* fmt);
void FLOG_INFO(const char* fmt, const int argc, ...);
void LOG_INFO(const char* fmt);
void FLOG_DEBUG(const char* fmt, const int argc, ...);
void LOG_DEBUG( const char* fmt);


#endif
