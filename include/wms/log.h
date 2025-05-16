#ifndef _WMS_LOG_H
#define _WMS_LOG_H

typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} log_level;

#ifndef LOG_LEVEL
    #ifdef NDEBUG
        #define LOG_LEVEL LOG_LEVEL_WARN  // prod: only WARN and ERROR
    #else
        #define LOG_LEVEL LOG_LEVEL_DEBUG // debug: show everything
    #endif
#endif

#ifndef __WMS_FILE_NAME__
#define __WMS_FILE_NAME__ "UNKNOWN"
#endif

#define LOG(level, fmt, ...) \
    do { \
        if (level >= LOG_LEVEL) { \
            log_internal(level, __WMS_FILE_NAME__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while (0)

#define LOG_DEBUG(fmt, ...) LOG(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) LOG(LOG_LEVEL_INFO,  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) LOG(LOG_LEVEL_WARN,  fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

void log_internal(log_level level, const char *file, int line, const char *fmt, ...);

#endif // _WMS_LOG_H

