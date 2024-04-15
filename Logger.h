#pragma once

#include <string>
#include "noncopyable.h"

enum LogLevel {
    INFO,
    ERROR,
    FATAL,
    DEBUG
};

class Logger : noncopyable {
public:
    //获取日志的单例
    static Logger& GetInstance();
    void SetLogLevel(LogLevel level) { m_loglevel = level; }
    //写日志
    void Log(std::string msg);

private:
    LogLevel m_loglevel;

};

//定义宏
#define LOG_INFO(logmsgformat, ...) \
    do { \
        Logger &logger = Logger::GetInstance(); \
        logger.SetLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.Log(buf); \
    } while(0);

#define LOG_ERROR(logmsgformat, ...) \
    do { \
        Logger &logger = Logger::GetInstance(); \
        logger.SetLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.Log(buf); \
    } while(0);

#define LOG_FATAL(logmsgformat, ...) \
    do { \
        Logger &logger = Logger::GetInstance(); \
        logger.SetLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.Log(buf); \
        exit(-1); \
    } while(0);

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgformat, ...) \
do { \
    Logger &logger = Logger::GetInstance(); \
    logger.SetLogLevel(DEBUG); \
    char buf[1024] = {0}; \
    snprintf(buf, 1024, logmsgformat, ##__VA_ARGS__); \
    logger.Log(buf); \
} while(0);
#else
#define LOG_DEBUG(logmsgformat, ...)
#endif