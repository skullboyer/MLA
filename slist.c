/**
 * @file slist.c
 * @author skull (skull.gu@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-08-03
 *
 * @copyright Copyright (c) 2023 skull
 *
 */
#include <stdio.h>
#include <stdint.h>
#include "slist.h"
#include "adapter.h"

#define TAG    "SLIST"

int slist_init(slist_head_t *p_head)
{
    CHECK(p_head != NULL, -0xFF);

    p_head->p_next = NULL;
    return 0;
}

int slist_add(slist_node_t *p_pos, slist_node_t *p_node)
{
    p_node->p_next = p_pos->p_next;
    p_pos->p_next = p_node;
    return 0;
}

int slist_add_head(slist_head_t *p_head, slist_node_t *p_node)
{
    return slist_add(p_head, p_node);
}

slist_node_t *slist_prev_get(slist_head_t *p_head, slist_node_t *p_pos)
{
    slist_node_t *p_tmp = p_head;
    while ((p_tmp != NULL) && (p_tmp->p_next != p_pos)) {
        p_tmp = p_tmp->p_next;
    }
    return p_tmp;
}

slist_node_t *slist_tail_get(slist_head_t *p_head)
{
    return slist_prev_get(p_head, NULL);
}

int slist_add_tail(slist_head_t *p_head, slist_node_t *p_node)
{
    slist_node_t *p_tmp = slist_tail_get(p_head);
    return slist_add(p_tmp, p_node);
}

int slist_del(slist_head_t *p_head, slist_node_t *p_node)
{
    slist_node_t *p_prev = slist_prev_get(p_head, p_node);
    if (p_prev) {
        p_prev->p_next = p_node->p_next;
        p_node->p_next = NULL;
        return 0;
    }
    return -1;
}

slist_node_t *slist_next_get(slist_node_t *p_pos)
{
    if (p_pos) {
        return p_pos->p_next;
    }
    return NULL;
}

slist_node_t *slist_begin_get(slist_head_t *p_head)
{
    return slist_next_get(p_head);
}

slist_node_t *slist_end_get(slist_head_t *p_head)
{
    return NULL;
}

void *slist_foreach(slist_head_t *p_head, slist_node_process_t pfn_node_process, void *p_arg)
{
    CHECK(p_head != NULL, NULL);
    CHECK(pfn_node_process != NULL, NULL);

    slist_node_t *p_tmp, *p_end;
    int ret = 0;
    p_tmp = slist_begin_get(p_head);
    p_end = slist_end_get(p_head);
    while (p_tmp != p_end) {
        ret = pfn_node_process(&p_arg, &p_tmp);
        if (ret) {
            return p_tmp;
        }
        p_tmp = slist_next_get(p_tmp);
    }
    return NULL;
}

static int slist_node_count(void **p_arg, slist_node_t **p_node)
{
    CHECK(*p_node != NULL, -0xFF);
    UNUSED(*p_node);

    (*(uint16_t *)(*p_arg))++;
    return 0;
}

int slist_node_count_get(slist_node_t *p_head)
{
    CHECK(p_head != NULL, -0xFF);

    uint16_t node_count = 0;
    slist_foreach(p_head, slist_node_count, &node_count);
    // LOGD("slist node count: %u", node_count);
    return node_count;
}