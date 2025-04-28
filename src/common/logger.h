#pragma once
#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <string>

class MyLogger {
public:
    static void init(const std::string& configFile) {
        try {
            log4cxx::PropertyConfigurator::configure(configFile);
        } catch (const log4cxx::helpers::Exception& e) {
            fprintf(stderr, "log4cxx init failed: %s\n", e.what());
        }
    }
    static log4cxx::LoggerPtr getLogger(const std::string& name) {
        return log4cxx::Logger::getLogger(name);
    }
};

#define LOG_TRACE(logger, msg) LOG4CXX_TRACE(logger, msg)
#define LOG_DEBUG(logger, msg) LOG4CXX_DEBUG(logger, msg)
#define LOG_INFO(logger, msg)  LOG4CXX_INFO(logger, msg)
#define LOG_WARN(logger, msg)  LOG4CXX_WARN(logger, msg)
#define LOG_ERROR(logger, msg) LOG4CXX_ERROR(logger, msg)