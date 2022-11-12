#include "Log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define LOG_LEVEL_LIST \
    X(LOG_DEBUG), \
    X(LOG_INFO),  \
    X(LOG_WARN),  \
    X(LOG_PANIC), \
    X(LOG_LEVEL_COUNT),

#define X(a) a
enum LogLevel { LOG_LEVEL_LIST };
#undef X
#define X(a) #a
const char *LogLevel_str[] = { LOG_LEVEL_LIST };
#undef X

static enum LogLevel level = LOG_INFO;

void Log_global_init(void) {
    /* 
     * set environment SSLIDE_TRACE to one of 
     * LOG_DEBUG
     * LOG_INFO
     * LOG_WARN
     * LOG_PANIC
     */
    level = LOG_INFO;
    char *s = getenv("SSLIDE_TRACE");
    if (!s) return;
    for (size_t i = 0; i < LOG_LEVEL_COUNT; i++) {
        if (strcmp(s, LogLevel_str[i]) == 0) {
            level = i;
            break;
        }
    }
}

void Debug(const char *fmt, ...) {
    if (level > LOG_DEBUG) return;
    fprintf(stderr, "[DEBUG]: ");
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
    fputc('\n', stderr);
    fflush(stderr);
}

void Info(const char *fmt, ...) {
    if (level > LOG_INFO) return;
    fprintf(stderr, "[INFO]: ");
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
    fputc('\n', stderr);
    fflush(stderr);
}

void Warn(const char *fmt, ...) {
    if (level > LOG_WARN) return;
    fprintf(stderr, "[WARN]: ");
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
    fputc('\n', stderr);
    fflush(stderr);
}

void Panic(const char *fmt, ...) {
    if (level > LOG_PANIC) return;
    fprintf(stderr, "[PANIC]: ");
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
    fputc('\n', stderr);
    fflush(stderr);
    abort();
}

