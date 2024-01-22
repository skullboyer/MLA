/**
 * @file slist.h
 * @author skull (skull.gu@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-08-03
 *
 * @copyright Copyright (c) 2023 skull
 *
 */
#pragma once

typedef struct _slist_node {
    struct _slist_node *p_next;
} slist_node_t;

typedef slist_node_t slist_head_t;
typedef int(*slist_node_process_t)(void **p_arg, slist_node_t **p_node);

int slist_init(slist_node_t *p_head);
int slist_add(slist_node_t *p_pos, slist_node_t *p_node);
int slist_add_tail(slist_node_t *p_head, slist_node_t *p_node);
int slist_add_head(slist_node_t *p_head, slist_node_t *p_node);
int slist_del(slist_node_t *p_head, slist_node_t *p_node);

slist_node_t *slist_prev_get(slist_node_t *p_head, slist_node_t *p_pos);
slist_node_t *slist_next_get(slist_node_t *p_pos);
slist_node_t *slist_tail_get(slist_node_t *p_head);
slist_node_t *slist_begin_get(slist_node_t *p_head);
slist_node_t *slist_end_get(slist_node_t *p_head);

void *slist_foreach(slist_node_t *p_head, slist_node_process_t pfn_node_process, void *p_arg);
int slist_node_count_get(slist_node_t *p_head);