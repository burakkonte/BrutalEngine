#include "brutal/core/logging.h"
#include <cstdio>
#include <cstdarg>

namespace brutal {

static FILE* g_log_file = nullptr;

void log_init() {
    g_log_file = fopen("brutal.log", "w");
}

void log_shutdown() {
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = nullptr;
    }
}

static void log_write(const char* level, const char* fmt, va_list args) {
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);
    printf("[%s] %s\n", level, buf);
    if (g_log_file) {
        fprintf(g_log_file, "[%s] %s\n", level, buf);
        fflush(g_log_file);
    }
}

void log_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_write("INFO", fmt, args);
    va_end(args);
}

void log_warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_write("WARN", fmt, args);
    va_end(args);
}

void log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_write("ERROR", fmt, args);
    va_end(args);
}

}
