#ifndef __UTILS_H
#define __UTILS_H

#include "config.h"

//#if (LOG_LEVEL & LOG_TRACE) == LOG_TRACE
    #define TRACE(filter, fmt, ...) if ((LOG_LEVEL & LOG_TRACE) == LOG_TRACE) \
        if (filter & LOG_FILTER)  printf("[TRACE @" #filter"] " fmt, ##__VA_ARGS__)
// #else
//     #define TRACE(filter, fmt, ...) 
// #endif
// 
// #if (LOG_LEVEL & LOG_DEBUG) == LOG_DEBUG
    #define DEBUG(filter, fmt, ...) if ((LOG_LEVEL & LOG_DEBUG) == LOG_DEBUG) \
        if (filter & LOG_FILTER) printf("[DEBUG @" #filter"] " fmt, ##__VA_ARGS__)
// #else
//     #define DEBUG(filter, fmt, ...) 
// #endif
// 
// #if (LOG_LEVEL & LOG_INFO) == LOG_INFO
    #define INFO(filter, fmt, ...) if ((LOG_LEVEL & LOG_INFO) == LOG_INFO) \
        if (filter & LOG_FILTER) printf("[INFO @" #filter"] " fmt, ##__VA_ARGS__)
// #else
//     #define INFO(filter, fmt, ...)
// #endif
// 
// #if (LOG_LEVEL & LOG_WARNING) == LOG_WARNING
    #define WARNING(filter, fmt, ...) if ((LOG_LEVEL & LOG_WARNING) == LOG_WARNING) \
        if (filter & LOG_FILTER) printf("[WARNING @" #filter"] " fmt, ##__VA_ARGS__)
// #else
//     #define WARNING(filter, fmt, ...)
// #endif
// 
// 
// #if (LOG_LEVEL & LOG_ERROR) == LOG_ERROR
    #define ERROR(filter, fmt, ...) if ((LOG_LEVEL & LOG_ERROR) == LOG_ERROR) \
        if (filter & LOG_FILTER) printf("[ERROR @" #filter"] " fmt, ##__VA_ARGS__)
// #else
//     #define ERROR(filter, fmt, ...)
// #endif
// #if (LOG_LEVEL & LOG_FATAL) == LOG_FATAL
    #define FATAL(filter, fmt, ...) if ((LOG_LEVEL & LOG_FATAL) == LOG_FATAL) \
        if (filter & LOG_FILTER) printf("[FATAL @" #filter"] " fmt, ##__VA_ARGS__)
// #else
//     #define FATAL(filter, fmt, ...)
// #endif

#endif /* __UTILS_H */
