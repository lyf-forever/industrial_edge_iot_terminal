/**
 * @file log_fw.c
 * @brief 结构化日志实现：环形缓冲 + 时间戳
 */

#include "log_fw.h"

#if LogUse

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define LOG_BUF_SIZE  4096

static uint8_t  s_buf[LOG_BUF_SIZE];
static volatile uint32_t s_write_idx = 0;
static volatile uint32_t s_read_idx = 0;
static SemaphoreHandle_t s_mtx = NULL;
static bool s_inited = false;

static uint32_t now_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

void log_fw_init(void *arg)
{
    (void)arg;
    memset(s_buf, 0, sizeof(s_buf));
    s_write_idx = 0;
    s_read_idx = 0;
    s_mtx = xSemaphoreCreateMutex();
    s_inited = true;
    ESP_LOGI("log_fw", "init ok (buf=%d)", LOG_BUF_SIZE);
}

void log_write(log_level_t level, const char *module, const char *fmt, ...)
{
    if (!s_inited || level > LOG_LEVEL_DEBUG) return;

    char msg[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    if (n < 0) n = 0;
    if (n > (int)sizeof(msg) - 1) n = (int)sizeof(msg) - 1;

    /* 构造条目：hdr(11B) + msg + '\0' */
    log_entry_hdr_t hdr;
    hdr.ts = now_ms();
    hdr.line = (uint16_t)((level << 12) | (module ? (uint8_t)module[0] : 0));
    hdr.level = (uint8_t)level;
    hdr.len = (uint8_t)(n + 1);   /* 含 '\0' */

    if (s_mtx) xSemaphoreTake(s_mtx, portMAX_DELAY);
    uint32_t entry_len = sizeof(hdr) + hdr.len;
    if (entry_len > LOG_BUF_SIZE) {  /* 单条过大丢弃 */
        if (s_mtx) xSemaphoreGive(s_mtx);
        return;
    }
    /* overwrite 模式：空间不足回绕覆盖 */
    for (uint32_t i = 0; i < entry_len; i++) {
        uint8_t byte = 0;
        if (i < sizeof(hdr)) {
            const uint8_t *p = (const uint8_t *)&hdr;
            byte = p[i];
        } else {
            byte = (uint8_t)msg[i - sizeof(hdr)];
        }
        s_buf[s_write_idx] = byte;
        s_write_idx = (s_write_idx + 1) % LOG_BUF_SIZE;
        if (s_write_idx == s_read_idx) {
            s_read_idx = (s_read_idx + 1) % LOG_BUF_SIZE;   /* 覆盖最旧 */
        }
    }
    if (s_mtx) xSemaphoreGive(s_mtx);
}

uint16_t log_export(log_level_t min_level, uint8_t *buf, uint16_t buf_size)
{
    if (!s_inited || buf == NULL || buf_size == 0) return 0;
    uint16_t out = 0;
    if (s_mtx) xSemaphoreTake(s_mtx, portMAX_DELAY);
    uint32_t idx = s_read_idx;
    while (idx != s_write_idx && out < buf_size) {
        log_entry_hdr_t hdr;
        uint32_t left = LOG_BUF_SIZE - idx;
        if (left < sizeof(hdr)) { idx = 0; continue; }   /* 跨边界跳过对齐 */
        memcpy(&hdr, &s_buf[idx], sizeof(hdr));
        if (hdr.len > 128 || hdr.level > LOG_LEVEL_DEBUG) { idx = (idx + 1) % LOG_BUF_SIZE; continue; }
        if (hdr.level >= (uint8_t)min_level) {
            uint32_t need = sizeof(hdr) + hdr.len;
            uint32_t can = (uint32_t)(buf_size - out);
            uint32_t cp = need < can ? need : can;
            /* 逐字节复制（支持环形回绕） */
            for (uint32_t i = 0; i < cp; i++) {
                buf[out++] = s_buf[(idx + i) % LOG_BUF_SIZE];
            }
        }
        idx = (idx + sizeof(hdr) + hdr.len) % LOG_BUF_SIZE;
    }
    if (s_mtx) xSemaphoreGive(s_mtx);
    return out;
}

uint8_t log_usage_percent(void)
{
    if (!s_inited) return 0;
    uint32_t used;
    if (s_mtx) xSemaphoreTake(s_mtx, portMAX_DELAY);
    used = (s_write_idx + LOG_BUF_SIZE - s_read_idx) % LOG_BUF_SIZE;
    if (s_mtx) xSemaphoreGive(s_mtx);
    return (uint8_t)((uint32_t)used * 100 / LOG_BUF_SIZE);
}

#endif /* LogUse */