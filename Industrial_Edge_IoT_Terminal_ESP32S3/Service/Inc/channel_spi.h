#ifndef __CHANNEL_SPI_H_
#define __CHANNEL_SPI_H_

/**
 * @file channel_spi.h
 * @brief HS-SPI 通道实现（架构 3.3 双链路冗余的 SPI 主链路）
 *
 * 基于 ESP32S3 SPI master（SPI2）实现高速主链路通道，
 * 与 GD32H7 端 SPI 从机对接。采用半双工主机模式：
 *  - TX：spi_transmit 发送帧；
 *  - RX：周期轮询 spi_receive（HS-SPI 从机通常以应答式交互），
 *    收到数据经 channel_rx_cb 回调。
 *
 * 引脚按板级原理图配置（默认：SCK=12, MOSI=13, MISO=11, CS=10）。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if ChannelUse

/* SPI 引脚配置 */
#define CH_SPI_SCK_GPIO   12
#define CH_SPI_MOSI_GPIO  13
#define CH_SPI_MISO_GPIO  11
#define CH_SPI_CS_GPIO    10
#define CH_SPI_CLK_HZ     1000000    /* HS-SPI 时钟（按对端能力调整） */

/* 初始化 SPI 通道并注册到通道表 */
void channel_spi_register(void);

#endif /* ChannelUse */

#endif /* __CHANNEL_SPI_H_ */