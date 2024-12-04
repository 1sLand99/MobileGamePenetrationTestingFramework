#ifndef LOG_H
#define LOG_H
#include <android/log.h>
#include <libgen.h>
#include <GlobalInfo.h>
#define TAG "libMyHook"

#if enableTempLog
#define LOGTMP(FMT, ...) __android_log_print(ANDROID_LOG_INFO, TAG, "[%s:%d]:" FMT, \
basename(__FILE__), __LINE__, ##__VA_ARGS__)
#endif

#define LOGI(FMT, ...) __android_log_print(ANDROID_LOG_INFO, TAG, "[%s:%d]:" FMT, \
basename(__FILE__), __LINE__, ##__VA_ARGS__)

#define LOGD(FMT, ...) __android_log_print(ANDROID_LOG_DEBUG, TAG, "[%s:%d]:" FMT, \
basename(__FILE__), __LINE__, ##__VA_ARGS__)

#define LOGW(FMT, ...) __android_log_print(ANDROID_LOG_WARN, TAG, "[%s:%d]:" FMT, \
basename(__FILE__), __LINE__, ##__VA_ARGS__)

#define LOGE(FMT, ...) __android_log_print(ANDROID_LOG_ERROR, TAG, "[%s:%d]:" FMT, \
basename(__FILE__), __LINE__, ##__VA_ARGS__)

#endif