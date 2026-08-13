#ifndef __LOG_FW_H_
#define __LOG_FW_H_

/**
 * @file log_fw.h
 * @brief 结构化日志框架（架构 3.9）
 *
 * 分级(ERROR/WARN/INFO/DEBUG) + 时间戳 + 模块名 + 环形缓冲落盘。
 * 日志事件走事件总线（EVT_SYS_LOG），可经 cloud_bridge 上云或导出。
 * 对齐 GD32 端 log_ringbuf.h 预留设计。
 *
 * 线程安全：写入持互斥锁；环形缓冲满时覆盖最旧（overwrite 模式）。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if LogUse

/* 日志级别 */
typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
} log_level_t;

/* 日志条目头部（环形缓冲存储格式） */
#pragma pack(push, 1)
typedef struct {
    uint32_t ts;        /* 毫秒时间戳 */
    uint16_t line;      /* 源行号 */
    uint8_t  level;     /* log_level_t */
    uint8_t  len;       /* 消息长度（不含头） */
} log_entry_hdr_t;
#pragma pack(pop)

/* ===================== API ===================== */

/* 初始化日志框架（SOFTWARE 阶段） */
void log_fw_init(void *arg);

/* 写入一条日志（自动附加时间戳与模块名） */
void log_write(log_level_t level, const char *module, const char *fmt, ...);

/* 便捷宏 */
#define LOG_E(mod, ...) log_write(LOG_LEVEL_ERROR, mod, __VA_ARGS__)
#define LOG_W(mod, ...) log_write(LOG_LEVEL_WARN,  mod, __VA_ARGS__)
#define LOG_I(mod, ...) log_write(LOG_LEVEL_INFO,  mod, __VA_ARGS__)
#define LOG_D(mod, ...) log_write(LOG_LEVEL_DEBUG, mod, __VA_ARGS__)

/* 导出日志（按级别过滤）到调用方缓冲区；返回字节数 */
uint16_t log_export(log_level_t min_level, uint8_t *buf, uint16_t buf_size);

/* 环形缓冲当前使用率（0-100） */
uint8_t log_usage_percent(void);

#endif /* LogUse */

#endif /* __LOG_FW_H_ */