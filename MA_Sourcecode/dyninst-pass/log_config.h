#ifndef __LOG_CONFIG_H
#define __LOG_CONFIG_H

enum LogLevel
{
    LOG_LEVEL_FATAL = 1 << 0,
    LOG_LEVEL_ERROR = 1 << 1,
    LOG_LEVEL_WARNING = 1 << 2,
    LOG_LEVEL_INFO = 1 << 3,
    LOG_LEVEL_DEBUG = 1 << 4,
    LOG_LEVEL_TRACE = 1 << 5,
};

#define LOG_LEVEL_CONFIG                                                                 \
    (LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_WARNING | LOG_LEVEL_INFO)
#define LOG_LEVEL_FILE_CONFIG                                                            \
    (LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_WARNING | LOG_LEVEL_INFO)

enum LogType
{
    LOG_FILTER_GENERAL = 1 << 0,
    LOG_FILTER_TAKEN_ADDRESS = 1 << 1,
    LOG_FILTER_CALL_TARGET = 1 << 2,
    LOG_FILTER_CALL_SITE = 1 << 3,
    LOG_FILTER_REACHING_ANALYSIS = 1 << 4,
    LOG_FILTER_LIVENESS_ANALYSIS = 1 << 5,
    LOG_FILTER_INSTRUMENTATION = 1 << 6,
    LOG_FILTER_BINARY_PATCHING = 1 << 7,
    LOG_FILTER_TYPE_ANALYSIS = 1 << 8,
};

#define LOG_FILTER_CONFIG                                                                \
    (LOG_FILTER_GENERAL | LOG_FILTER_TAKEN_ADDRESS | LOG_FILTER_CALL_TARGET |            \
     LOG_FILTER_CALL_SITE | LOG_FILTER_BINARY_PATCHING)
#define LOG_FILTER_FILE_CONFIG                                                           \
    (LOG_FILTER_GENERAL | LOG_FILTER_INSTRUMENTATION | LOG_FILTER_CALL_SITE | LOG_FILTER_TYPE_ANALYSIS)

#endif /* __LOG_CONFIG_H */
