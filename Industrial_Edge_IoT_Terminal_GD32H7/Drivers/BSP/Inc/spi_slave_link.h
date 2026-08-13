#ifndef __SPI_SLAVE_LINK_H_
#define __SPI_SLAVE_LINK_H_

/**
 * @file spi_slave_link.h
 * @brief GD32H7 HS-SPI 从机链路（架构 3.3 双链路冗余：SPI 主链路）
 *
 * 将 GD32H7 配置为 SPI 从机（SPI2），与 ESP32S3 端 SPI master 通道
 * 对接，构成 UART + SPI 双链路冗余的跨核高速主链路。
 *
 * 帧交互协议（半双工应答式）：
 *   - 主机发送：首字节 = 数据长度，后续为 link 协议帧（已转义）；
 *   - 从机接收完整帧后转交 event_ipc 的 link 解析器；
 *   - 从机待发送数据经 SPI 应答返回（tx_buf 预置）。
 *
 * 引脚（按板级原理图调整）：
 *   SPI2: SCK=PB13, MOSI=PB15, MISO=PB14, CS=PB12 (AF5)
 */

#include <stdint.h>
#include <stdbool.h>

/* ===================== API ===================== */

/* 初始化 SPI 从机链路（main 中调用，需在 event_bus/event_ipc 之后） */
void spi_slave_link_init(void);

/* 由调用方在主循环周期调用：轮询 SPI 接收并解析 */
void spi_slave_link_poll(void);

/* 发送一帧（经 SPI 应答给主机）；返回 0=已缓冲 1=忙 -1=失败 */
int spi_slave_link_send(const uint8_t *data, uint16_t len);

/* 已接收帧计数（诊断） */
uint32_t spi_slave_link_rx_count(void);

#endif /* __SPI_SLAVE_LINK_H_ */