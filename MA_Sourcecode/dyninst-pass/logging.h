#ifndef __LOGGING_H
#define __LOGGING_H

#include "log_config.h"

#define TRACE(filter, fmt, ...) if ((LOG_LEVEL & LOG_TRACE) == LOG_TRACE) \
    if (filter & LOG_FILTER)  printf("[TRACE @" #filter"] " fmt "\n", ##__VA_ARGS__)

#define DEBUG(filter, fmt, ...) if ((LOG_LEVEL & LOG_DEBUG) == LOG_DEBUG) \
    if (filter & LOG_FILTER) printf("[DEBUG @" #filter"] " fmt "\n", ##__VA_ARGS__)

#define INFO(filter, fmt, ...) if ((LOG_LEVEL & LOG_INFO) == LOG_INFO) \
    if (filter & LOG_FILTER) printf("[INFO @" #filter"] " fmt "\n", ##__VA_ARGS__)

#define WARNING(filter, fmt, ...) if ((LOG_LEVEL & LOG_WARNING) == LOG_WARNING) \
    if (filter & LOG_FILTER) printf("[WARNING @" #filter"] " fmt "\n", ##__VA_ARGS__)

#define ERROR(filter, fmt, ...) if ((LOG_LEVEL & LOG_ERROR) == LOG_ERROR) \
    if (filter & LOG_FILTER) printf("[ERROR @" #filter"] " fmt "\n", ##__VA_ARGS__)

#define FATAL(filter, fmt, ...) if ((LOG_LEVEL & LOG_FATAL) == LOG_FATAL) \
    if (filter & LOG_FILTER) printf("[FATAL @" #filter"] " fmt "\n", ##__VA_ARGS__)

#include <iomanip>
#include <sstream>
#include <string>

template <typename int_t> std::string int_to_hex(int_t value)
{
    std::stringstream ss;
    ss << std::showbase << std::setfill('0') << std::setw(sizeof(int_t) * 2) << std::hex << value;
    return ss.str();
}

template <typename object_t> std::string to_string(object_t &object);

template <typename object_t> std::string to_string(std::vector<object_t> &objects)
{
    std::stringstream ss;

    for (auto &&object : objects)
        ss << to_string<object_t>(object) << "\n";

    return ss.str();
}

#endif /* __LOGGING_H */
