#ifndef __LOG_CONFIG_H
#define __LOG_CONFIG_H

enum LogLevel
{
    LOG_FATAL = 1 << 0,
    LOG_ERROR = 1 << 1,
    LOG_WARNING = 1 << 2,
    LOG_INFO = 1 << 3,
    LOG_DEBUG = 1 << 4,
    LOG_TRACE = 1 << 5,
};

#define LOG_LEVEL (LOG_FATAL | LOG_ERROR | LOG_INFO | LOG_TRACE)

enum LogType
{
    LOG_FILTER_GENERAL = 1 << 0,
    LOG_FILTER_TAKEN_ADDRESS = 1 << 1,
    LOG_FILTER_CALL_TARGET = 1 << 2,
    LOG_FILTER_CALL_SITE = 1 << 3
};

#define LOG_FILTER (LOG_FILTER_CALL_SITE)

#endif /* __LOG_CONFIG_H */
