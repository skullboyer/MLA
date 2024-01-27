/**
 * @file log.c
 * @author skull (skull.gu@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-01-27
 *
 * @copyright Copyright (c) 2024 skull
 *
 */
#include <stdio.h>
#include <stdarg.h>
#include "adapter.h"

#define TAG    "LOG"
#define CFG_LOG_BACKEND_TERMINAL    1
#define CFG_LOG_BACKEND_FILE    1
#define CFG_LOG_BACKEND_FLASH    0

uint32_t BKDRHash(char *str)
{
    uint32_t seed = 131;  // 31 131 1313 13131 131313 etc..
    uint32_t hash = 0;

    CHECK(str != NULL, 0);
    while (*str) {
        hash = hash * seed + (*str++);
    }

    return (hash & 0x7FFFFFFF);
}

/**
 * @brief  log timestamp
 */
char *get_current_time(uint16_t *minute_second)
{
    static char currentTime[20];
    struct timeval now;
    gettimeofday(&now, NULL);
    struct tm* date = localtime(&now);
    strftime(currentTime, sizeof(currentTime), "%m/%d %H:%M:%S", date);
    snprintf(currentTime + 14, 5, ".%03ld", now.tv_usec / 1000);
    if (minute_second != NULL) {
        *minute_second = now.tv_sec*10 + now.tv_usec / 100000;
    }

    return currentTime;
}

/**
 * @brief log throttling
 */
bool log_throttling(char *file, uint16_t line, uint8_t log_hz)
{
    typedef struct {
        uint32_t hash;
        uint16_t time_ms;  // s + 0.1s
        uint16_t count;
    } log_throttling_info;

    static struct {
        uint8_t write;
        uint8_t read;  // reserved
        bool empty;
    } log_throttling_handle = {0, 0, true};

    #define LOG_THROTTLING_BUF_SIZE    (20)
    static log_throttling_info log_data[LOG_THROTTLING_BUF_SIZE] = {0};

    char buf[64] = {0};
    snprintf(buf, sizeof(buf) - 1, "%s:%u", file, line);
    uint32_t hash = BKDRHash(buf);

    uint16_t time_ms;
    char time_stamp[20];
    memcpy(time_stamp, get_current_time(&time_ms), sizeof(time_stamp));

    if (log_throttling_handle.empty) {
        log_data[0].hash = hash;
        log_data[0].time_ms = time_ms;
        log_throttling_handle.empty = false;
        log_throttling_handle.write = 1;
        return false;
    }

    for (uint8_t i = 0; i < LOG_THROTTLING_BUF_SIZE; i++) {
        if (hash == log_data[i].hash) {
            if (time_ms - log_data[i].time_ms <= 10/log_hz) {
                log_data[i].count += 1;
                log_throttling_handle.read = i;
                return true;
            } else {
                if (log_data[i].count > 0) {
                    OUTPUT("W>%s " "{%.8s} " "<%s: %u> ""discard times: %u\r\n", time_stamp, TAG, file, line, log_data[i].count);
                }
                memset(&log_data[i], 0, sizeof(log_throttling_info));
            }
        }
    }

    uint8_t index = log_throttling_handle.write;
    if (index >= LOG_THROTTLING_BUF_SIZE) {
        index = 0;
    }
    log_data[index].hash = hash;
    log_data[index].time_ms = time_ms;
    log_throttling_handle.write = index + 1;

    return false;
}

#if CFG_LOG_BACKEND_FILE
FILE *logFile = NULL;

static int init_log_file(void)
{
    logFile = fopen("Log.log", "w+");
    if (logFile == NULL) {
        printf("Failed to open log file.\n");
        return -1;
    }
    return 0;
}

int output_file(FILE *file, char *buf, uint16_t len)
{
    if (file != NULL) {
        fwrite(buf, len, 1, file);
        fflush(file);
    }

    return 0;
}
#endif

#if CFG_LOG_BACKEND_TERMINAL
int output_terminal(char *buf)
{
    printf("%s", buf);
    return 0;
}
#endif

#if CFG_LOG_BACKEND_FLASH
struct {
    uint32_t address;
    uint32_t length;
    uint32_t write;
    uint32_t read;
    bool ferrule;
} log_handle;

int init_log_flash(void)
{
    log_handle.address = FLASH_ADDRESS;
    log_handle.length = FLASH_RANGE;
    log_handle.write = FLASH_ADDRESS;
    log_handle.read = FLASH_ADDRESS;
    log_handle.ferrule = false;
    return 0;
}

int output_flash(char *buf, uint16_t len)
{
    if (log_handle.write + len > FLASH_ADDRESS + FLASH_RANGE) {
        flash_write(log_handle.write, buf, FLASH_ADDRESS + FLASH_RANGE - log_handle.write);
        uint16_t remain = log_handle.write + len - FLASH_ADDRESS - FLASH_RANGE;
        log_handle.write = FLASH_ADDRESS;
        flash_write(log_handle.write, buf, remain);
        log_handle.write += remain;
        log_handle.ferrule = true;
    } else {
        flash_write(log_handle.write, buf, len);
        log_handle.write += len;
    }

    return 0;
}

int sync_flash(char *buf)
{
    uint16_t len = 0;

    // 发生套圈
    if (log_handle.ferrule) {
        // 数据存在覆盖，以当前写地址开始回环读完
        if (log_handle.write > log_handle.read) {
            log_handle.read = log_handle.write;
            len = FLASH_RANGE;
        } else {
            // 未覆盖，以当前读地址开始回环读到写地址结束
            len = FLASH_ADDRESS + FLASH_RANGE - log_handle.read;
            len += log_handle.write - FLASH_ADDRESS;
        }
    } else {
        // 没有套圈，以当前读地址开始读到写地址结束
        if (log_handle.write > log_handle.read) {
            len = log_handle.write - log_handle.read;
        }
        // 读地址等于写地址，说明没有新数据
    }

    if (log_handle.read + len > FLASH_ADDRESS + FLASH_RANGE) {
        flash_read(log_handle.read, buf, FLASH_ADDRESS + FLASH_RANGE - log_handle.read);
        uint16_t remain = log_handle.read + len - FLASH_ADDRESS - FLASH_RANGE;
        log_handle.read = FLASH_ADDRESS;
        flash_read(log_handle.read, buf, remain);
        log_handle.read += remain;
        log_handle.ferrule = false;
    } else {
        flash_read(log_handle.read, buf, len);
        log_handle.read += len;
    }
}
#endif

int log_init(void)
{
    int ret = 0;
#if CFG_LOG_BACKEND_FILE
    ret =  init_log_file();
#endif
    return ret;
}

int log_deinit(void)
{
    int ret = 0;
#if CFG_LOG_BACKEND_FILE
    ret = fclose(logFile);
#endif
    return ret;
}

int log_out(const char *format, ...)
{
    int ret = 0;
    va_list args;
    va_start(args, format);
    char log_buffer[LOG_BUFFER_SIZE];
    vsnprintf(log_buffer, sizeof(log_buffer), format, args);
    uint16_t len = strlen(log_buffer);

#if CFG_LOG_BACKEND_TERMINAL
    ret = output_terminal(log_buffer);
#endif
#if CFG_LOG_BACKEND_FILE
    ret = output_file(logFile, log_buffer, len);
#endif
#if CFG_LOG_BACKEND_FLASH
    ret = output_flash(log_buffer, len);
#endif

    va_end(args);
    return ret;
}
