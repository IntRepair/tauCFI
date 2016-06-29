#ifndef __LOGGING_H
#define __LOGGING_H

#include "log_config.h"

#define TRACE(filter, fmt, ...) if ((LOG_LEVEL & LOG_TRACE) == LOG_TRACE) \
    if (filter & LOG_FILTER)  printf("[TRACE @" #filter"] " fmt, ##__VA_ARGS__)

#define DEBUG(filter, fmt, ...) if ((LOG_LEVEL & LOG_DEBUG) == LOG_DEBUG) \
	if (filter & LOG_FILTER) printf("[DEBUG @" #filter"] " fmt, ##__VA_ARGS__)

#define INFO(filter, fmt, ...) if ((LOG_LEVEL & LOG_INFO) == LOG_INFO) \
    if (filter & LOG_FILTER) printf("[INFO @" #filter"] " fmt, ##__VA_ARGS__)

#define WARNING(filter, fmt, ...) if ((LOG_LEVEL & LOG_WARNING) == LOG_WARNING) \
    if (filter & LOG_FILTER) printf("[WARNING @" #filter"] " fmt, ##__VA_ARGS__)

#define ERROR(filter, fmt, ...) if ((LOG_LEVEL & LOG_ERROR) == LOG_ERROR) \
    if (filter & LOG_FILTER) printf("[ERROR @" #filter"] " fmt, ##__VA_ARGS__)

#define FATAL(filter, fmt, ...) if ((LOG_LEVEL & LOG_FATAL) == LOG_FATAL) \
    if (filter & LOG_FILTER) printf("[FATAL @" #filter"] " fmt, ##__VA_ARGS__)

#endif /* __LOGGING_H */
