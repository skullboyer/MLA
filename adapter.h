/**
 * @file adapter.h
 * @author skull (skull.gu@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-08-03
 *
 * @copyright Copyright (c) 2023 skull
 *
 */
#pragma once

#include <sys/time.h>
#include <time.h>
#include "slist.h"
#include "mla.h"

#define mla_list_init                slist_init
#define mla_slist_foreach            slist_foreach
#define mla_list_add                 slist_add
#define mla_list_add_tail            slist_add_tail
#define mla_list_next_get            slist_next_get
#define mla_list_prev_get            slist_prev_get
#define mla_list_del                 slist_del
#define mla_list_node_count_get      slist_node_count_get
typedef slist_node_t    mla_list_node_t;

// V: view, VO: only view
enum {LOG_LEVEL, V, D, I, W, E, NO, VO, DO, IO, WO, EO};

#define FILTER    V  // log filtering level (exclude oneself)
#define LOGV(...)    LOG(V, __VA_ARGS__)
#define LOGD(...)    LOG(D, __VA_ARGS__)
#define LOGI(...)    LOG(I, __VA_ARGS__)
#define LOGW(...)    LOG(W, __VA_ARGS__)
#define LOGE(...)    LOG(E, __VA_ARGS__)

#define OUTPUT    log_out
#define TAG    "TAG"  // default tag
#define LOG_BUFFER_SIZE    (256)
#define LOG_HZ    (0)  // 0: not care, >1: max number of ouput per second

// Level - Date Time - {Tag} - <func: line> - message
// E>09/16 11:17:33.990 {TEST-sku} <test: 373> This is test.
#define LOG(level, ...) \
    do { \
        if (level + NO == FILTER) { \
        } else if (level < FILTER) { \
            break; \
        } \
        char tag[] = TAG; \
        if (log_control(tag)) break; \
        bool jump = false; \
        level == V ? : LOG_HZ == 0 ? : (jump = log_throttling(__FILENAME__, __LINE__, LOG_HZ)); \
        if (jump) break; \
        level == V ? : OUTPUT(#level">%s " "{%.8s} " "<%s: %u> ", get_current_time(NULL), tag, __FILENAME__, __LINE__); \
        OUTPUT(__VA_ARGS__); \
        level == V ? : OUTPUT("\r\n"); \
    } while(0)

#define __FILENAME__    (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1) : __FILE__)
#define UNUSED(x)    (void)(x)
#define ASSERT(expr)    (void)((!!(expr)) || (OUTPUT("%s assert fail: \"%s\" @ %s, %u\r\n", TAG, #expr, __FILE__, __LINE__), assert_abort()))
#define CHECK_MSG(expr, msg, ...) \
    do { \
        if (!(expr)) { \
            LOGE("check fail: \"%s\". %s @ %s, %u", #expr, msg, __FILE__, __LINE__); \
            return __VA_ARGS__; \
        } \
    } while(0)
#define CHECK_MSG_PRE(expr, ...)    CHECK_MSG(expr, "Error occurred", __VA_ARGS__)
#define VA_NUM_ARGS_IMPL(_1,_2,__N,...)    __N
#define GET_MACRO(...)    VA_NUM_ARGS_IMPL(__VA_ARGS__, CHECK_MSG, CHECK_MSG_PRE)
#define CHECK(expr, ...)    GET_MACRO(__VA_ARGS__)(expr, __VA_ARGS__)

uint32_t BKDRHash(char *str);
int log_init(void);
int log_deinit(void);
int log_out(const char *format, ...);
char *get_current_time(uint16_t *minute_second);
bool log_throttling(char *file, uint16_t line, uint8_t log_hz);
bool log_control(char *tag);
