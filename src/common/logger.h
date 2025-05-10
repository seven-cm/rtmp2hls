#pragma once
#include <log4cxx/helpers/exception.h>
#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>

#include <cstdarg>
#include <memory>
#include <string>
#include <type_traits>

class MyLogger
{
  public:
    static void init(const std::string &configFile)
    {
        try
        {
            log4cxx::PropertyConfigurator::configure(configFile);
        }
        catch (const log4cxx::helpers::Exception &e)
        {
            fprintf(stderr, "log4cxx init failed: %s\n", e.what());
        }
    }
    static log4cxx::LoggerPtr getLogger(const std::string &name) { return log4cxx::Logger::getLogger(name); }
    static log4cxx::LoggerPtr getDefaultLogger()
    {
        static log4cxx::LoggerPtr defaultLogger = log4cxx::Logger::getLogger("default");
        return defaultLogger;
    }
};

namespace logger_util
{
// 基础转换函数
inline const char *convert_arg(const std::string &arg) { return arg.c_str(); }

template <typename T> inline T &&convert_arg(T &&arg) { return std::forward<T>(arg); }

// 处理格式化字符串
inline std::string format_message(const char *fmt, ...)
{
    va_list args1, args2;
    va_start(args1, fmt);
    va_copy(args2, args1);

    const size_t size = vsnprintf(nullptr, 0, fmt, args1) + 1;
    va_end(args1);

    std::unique_ptr<char[]> buf(new char[size]);
    vsnprintf(buf.get(), size, fmt, args2);
    va_end(args2);

    return std::string(buf.get(), buf.get() + size - 1);
}

// 处理日志消息的分发
inline std::string process_log_message(const std::string &msg) { return msg; }

inline std::string process_log_message(const char *msg) { return std::string(msg); }

template <typename... Args> inline std::string process_log_message(const char *fmt, Args... args)
{
    return format_message(fmt, args...);
}
} // namespace logger_util

// 最终宏定义
#define LOG_TRACE(logger, ...) LOG4CXX_TRACE(logger, logger_util::process_log_message(__VA_ARGS__))
#define LOG_DEBUG(logger, ...) LOG4CXX_DEBUG(logger, logger_util::process_log_message(__VA_ARGS__))
#define LOG_INFO(logger, ...)  LOG4CXX_INFO(logger, logger_util::process_log_message(__VA_ARGS__))
#define LOG_WARN(logger, ...)  LOG4CXX_WARN(logger, logger_util::process_log_message(__VA_ARGS__))
#define LOG_ERROR(logger, ...) LOG4CXX_ERROR(logger, logger_util::process_log_message(__VA_ARGS__))