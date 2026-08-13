#ifndef __MEMPOOL_H_
#define __MEMPOOL_H_

/**
 * @file mempool.h
 * @brief 内存池 / 对象池（架构 3.6）
 *
 * 定长块 + 位图分配：O(1) 分配/归还、零碎片、可预测时延。
 * 适用于高频分配的对象（消息、JSON 缓冲、OTA 分片等）。
 * 缓冲区可位于内部 SRAM 或外部 PSRAM（ESP32S3 Octal PSRAM）。
 *
 * 线程安全：分配/归还持互斥锁（ISR 不可用，需 ISR 安全池可用
 * mempool_alloc_isr/free_isr，内部使用临界区）。
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "sys.h"

#if MemPoolUse

/* 池最大块数（位图大小限制） */
#define MEMPOOL_MAX_BLOCKS  512

/* 内存池句柄 */
typedef struct mempool mempool_t;

/* ===================== API ===================== */

/**
 * @brief 静态初始化一个内存池
 * @param pool    池句柄（调用方静态分配）
 * @param base    内存区基址（SRAM/PSRAM 均可）
 * @param buf_len 调用方提供的缓冲区总字节数（含数据区 + 位图区）
 * @param block_size 单块字节数（建议按对象 sizeof 对齐）
 * @param block_count 块数（<= MEMPOOL_MAX_BLOCKS）
 *
 * v2.0：新增 buf_len 校验。位图放置在数据区之后
 * （base + block_size*block_count 起，共 ceil(block_count/8) 字节），
 * 调用方必须保证 buf_len >= block_size*block_count + bitmap_bytes，
 * 否则初始化失败（位图写入越界到相邻内存）。
 */
void mempool_init(mempool_t *pool, void *base, uint16_t buf_len,
                  uint16_t block_size, uint16_t block_count);

/* 分配一块，失败返回 NULL */
void *mempool_alloc(mempool_t *pool);

/* 归还一块 */
void mempool_free(mempool_t *pool, void *block);

/* 统计：当前已分配块数 / 峰值块数 / 总块数 */
uint16_t mempool_used(mempool_t *pool);
uint16_t mempool_peak(mempool_t *pool);
uint16_t mempool_count(mempool_t *pool);

/* 线程安全版本（默认即线程安全，_isr 供中断上下文使用） */
void *mempool_alloc_isr(mempool_t *pool);
void mempool_free_isr(mempool_t *pool, void *block);

/* init 表接入占位（mempool 无全局态） */
void mempool_init_stub(void *arg);

#endif /* MemPoolUse */

#endif /* __MEMPOOL_H_ */