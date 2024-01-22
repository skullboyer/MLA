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

#define mla_list_init                slist_init
#define mla_slist_foreach            slist_foreach
#define mla_list_add                 slist_add
#define mla_list_add_tail            slist_add_tail
#define mla_list_next_get            slist_next_get
#define mla_list_prev_get            slist_prev_get
#define mla_list_del                 slist_del
#define mla_list_node_count_get      slist_node_count_get
typedef slist_node_t    mla_list_node_t;

#undef DEBUG
#define DEBUG    1

// V: view, VO: only view
enum {LOG_LEVEL, V, D, I, W, E, NO, VO, DO, IO, WO, EO};

#define FILTER    V  // log filtering level (exclude oneself)
#define LOGV(...)    LOG(V, __VA_ARGS__)
#define LOGD(...)    LOG(D, __VA_ARGS__)
#define LOGI(...)    LOG(I, __VA_ARGS__)
#define LOGW(...)    LOG(W, __VA_ARGS__)
#define LOGE(...)    LOG(E, __VA_ARGS__)

#define OUTPUT    printf

// Level - Date Time - {Tag} - <func: line> - message
// E>09/16 11:17:33.990 {TEST-sku} <test: 373> This is test.
#if DEBUG
#define LOG(level, ...) \
    do { \
        if (level + NO == FILTER) { \
        } else if (level < FILTER) { \
            break; \
        } \
        level == V ? : OUTPUT(#level">%s " "{%.8s} " "<%s: %u> ", getCurrentTime(), TAG, __FILENAME__, __LINE__); \
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
#else
#define LOG(...)        (void)0
#define ASSERT(expr)    (void)0
#endif

static char *getCurrentTime(void)
{
    static char currentTime[20];
    struct timeval now;
    gettimeofday(&now, NULL);
    struct tm* date = localtime(&now);
    strftime(currentTime, sizeof(currentTime), "%m/%d %H:%M:%S", date);
    sprintf(currentTime + 14, ".%03ld", now.tv_usec / 1000);

    return currentTime;
}
