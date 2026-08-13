/**
 * @file log_ringbuf.c
 * @brief 结构化日志环形缓冲（架构 3.9，GD32H7 端激活）
 *
 * 恢复预留实现：分级日志 + 时间戳 + 环形缓冲，头部布局与
 * ESP32S3 端 log_fw.h 的 log_entry_hdr_t 完全一致（ts4+line2+level1+len1），
 * 便于两端日志跨核汇聚与统一解析。
 */
#include "log_ringbuf.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "gd32h7xx_it.h"

log_ringbuf_t g_log_rb;

void log_ringbuf_init(uint8_t *buf, uint32_t size, bool overwrite) {
    g_log_rb.buffer    = buf;
    g_log_rb.size      = size;
    g_log_rb.write_idx = 0;
    g_log_rb.read_idx  = 0;
    g_log_rb.lost_cnt  = 0;
    g_log_rb.overwrite = overwrite;
}

static uint32_t log_ringbuf_write(uint8_t *data, uint32_t len) {
    uint32_t w = g_log_rb.write_idx;
    uint32_t r = g_log_rb.read_idx;
    uint32_t free_space = (w >= r) ? (g_log_rb.size - (w - r)) : (r - w);

    if (len > free_space) {
        if (g_log_rb.overwrite) {
            uint32_t need = len - free_space;
            if (need >= g_log_rb.size) {
                g_log_rb.lost_cnt++;
                return 0;
            }
            g_log_rb.read_idx = (r + need) % g_log_rb.size;
        } else {
            g_log_rb.lost_cnt++;
            return 0;
        }
    }

    uint32_t first_chunk = g_log_rb.size - w;
    if (first_chunk > len) first_chunk = len;
    memcpy(&g_log_rb.buffer[w], data, first_chunk);
    if (len > first_chunk) {
        memcpy(g_log_rb.buffer, data + first_chunk, len - first_chunk);
    }
    g_log_rb.write_idx = (w + len) % g_log_rb.size;
    return len;
}

void log_printf(uint8_t level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int msg_len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (msg_len <= 0) return;

    uint32_t total_len = sizeof(log_entry_hdr_t) + msg_len + 1;
    if (total_len > 256) return;                      /* 单条上限保护 */

    uint8_t temp_buf[256];
    log_entry_hdr_t *hdr = (log_entry_hdr_t *)temp_buf;
    hdr->ts        = get_tick();
    hdr->line      = 0;
    hdr->level     = level;
    hdr->len       = (uint8_t)msg_len;

    char *msg_start = (char *)(temp_buf + sizeof(log_entry_hdr_t));
    va_start(args, fmt);
    vsnprintf(msg_start, msg_len + 1, fmt, args);
    va_end(args);

    log_ringbuf_write(temp_buf, total_len);
}

void log_export_all(void) {
    uint32_t r = g_log_rb.read_idx;
    uint32_t w = g_log_rb.write_idx;
    while (r != w) {
        /* v2.0：条目可能跨环（写侧分段写入），按环顺序逐字节拷贝到
         * 局部缓冲再解析，避免对跨环条目直接指针读造成越界/乱码 */
        uint8_t tmp[256];
        uint32_t n = 0;
        uint32_t idx = r;
        while (n < sizeof(tmp) && idx != w) {
            tmp[n++] = g_log_rb.buffer[idx];
            idx = (idx + 1) % g_log_rb.size;
        }
        if (n < sizeof(log_entry_hdr_t)) break;   /* 条目不完整：退出 */

        log_entry_hdr_t *hdr = (log_entry_hdr_t *)tmp;
        uint32_t entry_len = sizeof(log_entry_hdr_t) + hdr->len + 1;
        if (hdr->len == 0 || entry_len > sizeof(tmp)) {
            g_log_rb.read_idx = w;   /* 数据损坏：放弃剩余，避免死循环 */
            break;
        }
        printf("[%lu][%u] %s\n", (unsigned long)hdr->ts, hdr->level,
               (char *)tmp + sizeof(log_entry_hdr_t));
        r = (r + entry_len) % g_log_rb.size;
        g_log_rb.read_idx = r;
    }
}