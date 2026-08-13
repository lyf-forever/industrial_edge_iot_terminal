#ifndef __CHANNEL_H_
#define __CHANNEL_H_

/**
 * @file channel.h
 * @brief 通道抽象（架构 3.3）
 *
 * 将物理链路（UART/SPI）抽象为统一 channel_t 接口：
 *   - send：发送字节流
 *   - set_rx_cb：注册接收回调
 *   - link_up：链路健康查询
 * 使 link 协议层不绑死 UART，可平滑迁移到 HS-SPI 双链路冗余。
 *
 * 设计要点：
 *  - 支持多通道注册（channel_register）；
 *  - 活动通道可热切换（channel_select），切换计数可查；
 *  - UART 通道为本实现；SPI 通道为占位（硬件驱动后续补充）。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if ChannelUse

/* 接收回调 */
typedef void (*channel_rx_cb_t)(const uint8_t *data, uint16_t len, void *user);

/* 通道接口 */
typedef struct {
    const char *name;                       /* 通道名 */
    int      (*send)(const uint8_t *d, uint16_t n);
    void     (*set_rx_cb)(channel_rx_cb_t cb, void *user);
    uint8_t  (*link_up)(void);              /* 1=链路正常 */
    void     (*deinit)(void);
} channel_t;

/* ===================== API ===================== */

/* 注册一个通道，返回通道句柄(0..CHANNEL_MAX-1)，-1 失败 */
int8_t channel_register(const channel_t *ch);

/* 选择活动通道（热切换） */
bool channel_select(int8_t idx);

/* 当前活动通道 */
const channel_t *channel_active(void);
int8_t channel_active_idx(void);

/* 已注册通道数（v2.0：热切换依据，避免硬编码 %2） */
int8_t channel_count(void);

/* 经活动通道发送 */
int channel_send(const uint8_t *d, uint16_t n);

/* 设置活动通道接收回调 */
void channel_set_rx_cb(channel_rx_cb_t cb, void *user);

/* 活动通道链路状态 */
bool channel_is_up(void);

/* 切换计数（诊断） */
uint32_t channel_switch_count(void);

/* ===================== 内置通道 ===================== */
/* UART 通道：将 bsp_uart 封装为 channel（实现于 channel_uart.c 或本文件） */
void channel_uart_register(void);

/* SPI 通道：占位（HS-SPI 驱动后续实现），注册后 send 返回 -1 */
void channel_spi_register_stub(void);

/* init 表接入：注册内置通道 */
void channel_init(void *arg);

#endif /* ChannelUse */

#endif /* __CHANNEL_H_ */