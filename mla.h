/**
 * @file mla.h
 * @author skull (skull.gu@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-08-03
 *
 * @copyright Copyright (c) 2023 skull
 *
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* MLA内部使用的内存管理接口 */
#define MLA_MALLOC(size)    malloc(size)
#define MLA_FREE(addr)      free(addr)

/* 对外提供使用的内存泄漏检查的分配释放接口 */
#define PORT_MALLOC(size)    MlaMalloc(size, __FILENAME__, __func__, __LINE__)
#define PORT_FREE(addr)      MlaFree(addr, __FILENAME__, __func__, __LINE__)

void MlaInit(void);
int MlaOutput(void);
void *MlaMalloc(uint32_t size, char *file, char *func, uint16_t line);
void MlaFree(void *addr, char *file, char *func, uint16_t line);
