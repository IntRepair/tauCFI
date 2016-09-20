#ifndef __LOGGING_H
#define __LOGGING_H

#include "log_config.h"

class FileLogger
{
    FileLogger() { file = fopen("patharmor.log", "w"); }

    ~FileLogger() { fclose(file); }

    FILE *file;

  public:
    static FILE *log_file()
    {
        static FileLogger logger;
        return logger.file;
    }
};

#define LOG_TRACE(filter, fmt, ...)                                                                \
    {                                                                                              \
        if ((LOG_LEVEL_CONFIG & LOG_LEVEL_TRACE) == LOG_LEVEL_TRACE)                               \
            if (filter & LOG_FILTER_CONFIG)                                                        \
                printf("[LOG_TRACE @" #filter "] " fmt "\n", ##__VA_ARGS__);                       \
        if ((LOG_LEVEL_FILE_CONFIG & LOG_LEVEL_TRACE) == LOG_LEVEL_TRACE)                          \
            if (filter & LOG_FILTER_FILE_CONFIG)                                                   \
                fprintf(FileLogger::log_file(), "[LOG_TRACE @" #filter "] " fmt "\n",              \
                        ##__VA_ARGS__);                                                            \
    }

#define LOG_DEBUG(filter, fmt, ...)                                                                \
    {                                                                                              \
        if ((LOG_LEVEL_CONFIG & LOG_LEVEL_DEBUG) == LOG_LEVEL_DEBUG)                               \
            if (filter & LOG_FILTER_CONFIG)                                                        \
                printf("[LOG_DEBUG @" #filter "] " fmt "\n", ##__VA_ARGS__);                       \
        if ((LOG_LEVEL_FILE_CONFIG & LOG_LEVEL_DEBUG) == LOG_LEVEL_DEBUG)                          \
            if (filter & LOG_FILTER_FILE_CONFIG)                                                   \
                fprintf(FileLogger::log_file(), "[LOG_TRACE @" #filter "] " fmt "\n",              \
                        ##__VA_ARGS__);                                                            \
    }

#define LOG_INFO(filter, fmt, ...)                                                                 \
    {                                                                                              \
        if ((LOG_LEVEL_CONFIG & LOG_LEVEL_INFO) == LOG_LEVEL_INFO)                                 \
            if (filter & LOG_FILTER_CONFIG)                                                        \
                printf("[LOG_INFO @" #filter "] " fmt "\n", ##__VA_ARGS__);                        \
        if ((LOG_LEVEL_FILE_CONFIG & LOG_LEVEL_TRACE) == LOG_LEVEL_TRACE)                          \
            if (filter & LOG_FILTER_FILE_CONFIG)                                                   \
                fprintf(FileLogger::log_file(), "[LOG_TRACE @" #filter "] " fmt "\n",              \
                        ##__VA_ARGS__);                                                            \
    }

#define LOG_WARNING(filter, fmt, ...)                                                              \
    {                                                                                              \
        if ((LOG_LEVEL_CONFIG & LOG_LEVEL_WARNING) == LOG_LEVEL_WARNING)                           \
            if (filter & LOG_FILTER_CONFIG)                                                        \
                printf("[LOG_WARNING @" #filter "] " fmt "\n", ##__VA_ARGS__);                     \
        if ((LOG_LEVEL_FILE_CONFIG & LOG_LEVEL_WARNING) == LOG_LEVEL_WARNING)                      \
            if (filter & LOG_FILTER_FILE_CONFIG)                                                   \
                fprintf(FileLogger::log_file(), "[LOG_TRACE @" #filter "] " fmt "\n",              \
                        ##__VA_ARGS__);                                                            \
    }

#define LOG_ERROR(filter, fmt, ...)                                                                \
    {                                                                                              \
        if ((LOG_LEVEL_CONFIG & LOG_LEVEL_ERROR) == LOG_LEVEL_ERROR)                               \
            if (filter & LOG_FILTER_CONFIG)                                                        \
                printf("[LOG_ERROR @" #filter "] " fmt "\n", ##__VA_ARGS__);                       \
        if ((LOG_LEVEL_FILE_CONFIG & LOG_LEVEL_ERROR) == LOG_LEVEL_ERROR)                          \
            if (filter & LOG_FILTER_FILE_CONFIG)                                                   \
                fprintf(FileLogger::log_file(), "[LOG_TRACE @" #filter "] " fmt "\n",              \
                        ##__VA_ARGS__);                                                            \
    }

#define LOG_FATAL(filter, fmt, ...)                                                                \
    {                                                                                              \
        if ((LOG_LEVEL_CONFIG & LOG_LEVEL_FATAL) == LOG_LEVEL_FATAL)                               \
            if (filter & LOG_FILTER_CONFIG)                                                        \
                printf("[LOG_LEVEL_FATAL @" #filter "] " fmt "\n", ##__VA_ARGS__);                 \
        if ((LOG_LEVEL_FILE_CONFIG & LOG_LEVEL_FATAL) == LOG_LEVEL_FATAL)                          \
            if (filter & LOG_FILTER_FILE_CONFIG)                                                   \
                fprintf(FileLogger::log_file(), "[LOG_TRACE @" #filter "] " fmt "\n",              \
                        ##__VA_ARGS__);                                                            \
    }

#endif /* __LOGGING_H */
