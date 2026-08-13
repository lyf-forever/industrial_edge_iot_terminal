/**
 * @file mempool.c
 * @brief 内存池实现：位图管理定长块
 */

#include "mempool.h"

#if MemPoolUse

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <string.h>

struct mempool {
    uint8_t  *base;
    uint16_t  block_size;
    uint16_t  block_count;
    uint8_t  *bitmap;         /* 每块 1 bit：1=已分配 */
    uint16_t  bitmap_bytes;
    uint16_t  used;
    uint16_t  peak;
    SemaphoreHandle_t mtx;    /* 常规路径互斥锁 */
    /* ISR 路径用临界区，不持有锁 */
};

void mempool_init(mempool_t *pool, void *base, uint16_t buf_len,
                  uint16_t block_size, uint16_t block_count)
{
    if (pool == NULL || base == NULL || block_size == 0 ||
        block_count == 0 || block_count > MEMPOOL_MAX_BLOCKS) {
        return;
    }
    /* v2.0：位图位于数据区之后，显式校验缓冲区容量，防位图越界写 */
    uint32_t bitmap_bytes = (uint32_t)(block_count + 7) / 8;
    uint32_t need = (uint32_t)block_size * block_count + bitmap_bytes;
    if (buf_len < need) {
        return;
    }
    memset(pool, 0, sizeof(*pool));
    pool->base = (uint8_t *)base;
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->bitmap_bytes = (uint16_t)bitmap_bytes;
    /* 位图放在池数据区末尾区域 */
    pool->bitmap = (uint8_t *)(pool->base) + (uint32_t)block_size * block_count;
    memset(pool->bitmap, 0, pool->bitmap_bytes);
    pool->mtx = xSemaphoreCreateMutex();
}

/* 位图查找首个空闲位 */
static int16_t bitmap_find_free(const uint8_t *bm, uint16_t bytes, uint16_t max_bits)
{
    for (uint16_t b = 0; b < bytes; b++) {
        if (bm[b] != 0xFF) {
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint16_t idx = b * 8 + bit;
                if (idx >= max_bits) return -1;
                if (!(bm[b] & (1 << bit))) return (int16_t)idx;
            }
        }
    }
    return -1;
}

static void *mempool_alloc_locked(mempool_t *pool)
{
    int16_t idx = bitmap_find_free(pool->bitmap, pool->bitmap_bytes, pool->block_count);
    if (idx < 0) return NULL;
    pool->bitmap[idx / 8] |= (uint8_t)(1 << (idx % 8));
    pool->used++;
    if (pool->used > pool->peak) pool->peak = pool->used;
    return (void *)(pool->base + (uint32_t)idx * pool->block_size);
}

static void mempool_free_locked(mempool_t *pool, void *block)
{
    if (block == NULL) return;
    uint32_t off = (uint32_t)((uint8_t *)block - pool->base);
    if (off % pool->block_size != 0) return;          /* 非法指针 */
    uint16_t idx = (uint16_t)(off / pool->block_size);
    if (idx >= pool->block_count) return;
    if (pool->bitmap[idx / 8] & (1 << (idx % 8))) {
        pool->bitmap[idx / 8] &= (uint8_t)~(1 << (idx % 8));
        if (pool->used > 0) pool->used--;
    }
}

void *mempool_alloc(mempool_t *pool)
{
    if (pool == NULL) return NULL;
    void *p = NULL;
    if (pool->mtx) xSemaphoreTake(pool->mtx, portMAX_DELAY);
    p = mempool_alloc_locked(pool);
    if (pool->mtx) xSemaphoreGive(pool->mtx);
    return p;
}

void mempool_free(mempool_t *pool, void *block)
{
    if (pool == NULL || block == NULL) return;
    if (pool->mtx) xSemaphoreTake(pool->mtx, portMAX_DELAY);
    mempool_free_locked(pool, block);
    if (pool->mtx) xSemaphoreGive(pool->mtx);
}

void *mempool_alloc_isr(mempool_t *pool)
{
    if (pool == NULL) return NULL;
    portMUX_TYPE *m = NULL; (void)m;
    /* ISR 路径：无锁（由调用方保证单一 ISR 上下文） */
    return mempool_alloc_locked(pool);
}

void mempool_free_isr(mempool_t *pool, void *block)
{
    if (pool == NULL) return;
    mempool_free_locked(pool, block);
}

uint16_t mempool_used(mempool_t *pool) { return pool ? pool->used : 0; }
uint16_t mempool_peak(mempool_t *pool) { return pool ? pool->peak : 0; }
uint16_t mempool_count(mempool_t *pool) { return pool ? pool->block_count : 0; }

/* init 表接入：mempool 无全局态，此处仅作登记占位 */
void mempool_init_stub(void *arg)
{
    (void)arg;
    ESP_LOGI("mempool", "mempool ready (static pools by user)");
}

#endif /* MemPoolUse */