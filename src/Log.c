#include "Log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define LOG_LEVEL_LIST \
    X(LogDebug), \
    X(LogInfo),  \
    X(LogWarn),  \
    X(LogPanic), \
    X(LogLevel_Count),

#define X(a) a
enum LogLevel { LOG_LEVEL_LIST };
#undef X
#define X(a) #a
const char *LogLevel_str[] = { LOG_LEVEL_LIST };
#undef X

static enum LogLevel level = LogInfo;

void Log_global_init(void) {
    level = LogInfo;
    char *s = getenv("SSLIDE_TRACE");
    if (!s) return;
    for (size_t i = 0; i < LogLevel_Count; i++) {
        if (strcmp(s, LogLevel_str[i]) == 0) {
            level = i;
            break;
        }
    }
}

void Debug(const char *fmt, ...) {
    if (level > LogDebug) return;
    fprintf(stderr, "[DEBUG]: ");
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
    fputc('\n', stderr);
    fflush(stderr);
}

void Info(const char *fmt, ...) {
    if (level > LogInfo) return;
    fprintf(stderr, "[INFO]: ");
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
    fputc('\n', stderr);
    fflush(stderr);
}

void Warn(const char *fmt, ...) {
    if (level > LogWarn) return;
    fprintf(stderr, "[WARN]: ");
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
    fputc('\n', stderr);
    fflush(stderr);
}

void Panic(const char *fmt, ...) {
    if (level > LogPanic) return;
    fprintf(stderr, "[PANIC]: ");
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
    fputc('\n', stderr);
    fflush(stderr);
    abort();
}

