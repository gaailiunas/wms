#include <wms/log.h>

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static const char *level_to_string(log_level level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO: return "INFO";
        case LOG_LEVEL_WARN: return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void log_internal(log_level level, const char *file, int line, const char *fmt, ...) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    char fileinfo[64];
    snprintf(fileinfo, sizeof(fileinfo), "%s:%d", file, line);

    fprintf(stderr, "[%s] [%-5s] %-10s | ", timestamp, level_to_string(level), fileinfo);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}

