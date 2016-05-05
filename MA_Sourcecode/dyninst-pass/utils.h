#ifndef __UTILS_H
#define __UTILS_H

#ifdef LOG_DEBUG
#define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__);
#else
#define LOG_DEBUG(fmt, ...)
#endif

#define LOG_WARNING(fmt, ...) printf("[WARNING] " fmt, ##__VA_ARGS__);

#define LOG_INFO(fmt, ...) printf("[INFO] " fmt, ##__VA_ARGS__);

#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt, ##__VA_ARGS__);

#endif /* __UTILS_H */
