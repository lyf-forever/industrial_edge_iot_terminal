#ifndef LOG_RINGBUF_H
#define LOG_RINGBUF_H

#include <stdint.h>
#include <stdbool.h>

#define LOG_BUF_SIZE   (64 * 1024)  // 64KB，可根据需要调整
#define LOG_LEVEL_ERROR  0
#define LOG_LEVEL_WARN   1
#define LOG_LEVEL_INFO   2
#define LOG_LEVEL_DEBUG  3

// 日志条目头部（紧凑对齐）
typedef struct {
    uint32_t timestamp;   // 系统 tick 或 CPU 周期
    uint16_t line;        // 代码行号
    uint8_t  level;       // 日志级别
    uint8_t  len;         // 消息长度（不含 '\0'）
} log_entry_hdr_t;

// 环形缓冲区控制块
typedef struct {
    uint8_t  *buffer;
    uint32_t  size;
    volatile uint32_t write_idx;
    volatile uint32_t read_idx;
    volatile uint32_t lost_cnt;
    bool      overwrite;   // 满时是否覆盖旧数据
} log_ringbuf_t;

extern log_ringbuf_t g_log_rb;

void log_ringbuf_init(uint8_t *buf, uint32_t size, bool overwrite);
void log_printf(uint8_t level, const char *fmt, ...);
void log_export_all(void);   // 通过调试器或外设导出（需用户实现底层）

#endif