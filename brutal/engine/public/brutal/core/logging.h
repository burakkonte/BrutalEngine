#ifndef BRUTAL_CORE_LOGGING_H
#define BRUTAL_CORE_LOGGING_H

namespace brutal {
    void log_init();
    void log_shutdown();
    void log_info(const char* fmt, ...);
    void log_warn(const char* fmt, ...);
    void log_error(const char* fmt, ...);
}

#define LOG_INFO(...) brutal::log_info(__VA_ARGS__)
#define LOG_WARN(...) brutal::log_warn(__VA_ARGS__)
#define LOG_ERROR(...) brutal::log_error(__VA_ARGS__)

#endif
