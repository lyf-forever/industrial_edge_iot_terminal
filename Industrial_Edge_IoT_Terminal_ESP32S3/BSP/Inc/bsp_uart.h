#ifndef __BSP_UART_H_
#define __BSP_UART_H_

/**
 * @file bsp_uart.h
 * @brief 板级 UART 驱动：ESP32S3 与 GD32H7 之间的串口链路
 *
 * 使用 ESP-IDF UART 驱动，UART1 端口，带 RX 环形缓冲与事件队列。
 * 上层（link 服务）通过 bsp_uart_rx_task 接收字节流并送入协议解析器，
 * 通过 bsp_uart_send 发送已构建的帧。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if LinkUse

/* UART 端口与引脚配置 */
#define BSP_UART_PORT        1            /* UART1 (UART0 为控制台) */
#define BSP_UART_TX_PIN       17           /* ESP32S3 TX -> GD32H7 RX */
#define BSP_UART_RX_PIN       18           /* ESP32S3 RX <- GD32H7 TX */
#define BSP_UART_BAUDRATE     115200       /* 链路波特率 */
#define BSP_UART_RX_BUF_SIZE  512          /* 内部 RX 环形缓冲(字节) */
#define BSP_UART_TX_BUF_SIZE  256          /* 内部 TX 缓冲(字节) */
#define BSP_UART_EVT_QUEUE_LEN 20          /* UART 事件队列深度 */
#define BSP_UART_RX_CHUNK     256          /* 单次读取最大字节数 */

/* UART 事件类型（用于事件队列回调） */
typedef enum {
    BSP_UART_EVT_DATA = 0,    /* 收到数据 */
    BSP_UART_EVT_BREAK,       /* 检测到 break */
    BSP_UART_EVT_ERR          /* 帧错误/奇偶校验错误 */
} bsp_uart_evt_t;

/* 接收回调：上层注册，收到一段字节流时被调用 */
typedef void (*bsp_uart_rx_cb_t)(const uint8_t *data, uint16_t len, void *user_data);

/* ===================== API 声明 =============================== */

/* 初始化 UART 驱动（在初始化表 HARDWARE 阶段调用） */
void bsp_uart_init(void *arg);

/* 发送数据（阻塞直到写入 TX 缓冲，返回写入字节数） */
int bsp_uart_send(const uint8_t *data, uint16_t len);

/* 注册接收回调 */
void bsp_uart_set_rx_callback(bsp_uart_rx_cb_t cb, void *user_data);

/* 启动 RX 处理任务（在任务表注册，由 StartTask 创建） */
void bsp_uart_rx_task(void *arg);

#endif /* LinkUse */

#endif /* __BSP_UART_H_ */