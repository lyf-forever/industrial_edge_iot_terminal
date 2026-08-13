/**
 * @file spi_slave_link.c
 * @brief GD32H7 SPI 从机链路实现（封装 driver_spi 从机 DMA）
 *
 * 基于 driver_spi 的从机描述结构：SPI2 + 从机模式 + DMA 收发。
 * 接收完整帧后调用 event_ipc_feed 进入 link 解析（与 UART3 同管线），
 * 实现"双链路冗余：任意链路收到的帧都进同一解析器"。
 *
 * 非阻塞设计（code_check v2.0 P0 修复）：
 *   - 从机无法得知主机何时发起事务，因此 RX DMA 常驻武装（256B）；
 *   - poll 仅做非阻塞标志查询：DMA 完成（FTF）或部分接收计数稳定即处理，
 *     绝不在主循环内等待 SPI 传输完成（原实现内部 driver_spi 与 driver_dma
 *     的等待函数最长阻塞 100ms/300ms，会拖垮 UART3 主链路）；
 *   - 处理完一帧立即重新武装，保证主机下次事务到达时 DMA 处于就绪态。
 */
#include "spi_slave_link.h"
#include "driver_spi.h"
#include "driver_gpio.h"
#include "event_ipc.h"
#include "gd32h7xx_dma.h"
#include "public.h"
#include <string.h>

/* ---------------- SPI2 从机引脚（GD32H7: PB13 SCK, PB15 MOSI, PB14 MISO, PB12 CS, AF5） ---------------- */
SPI_SLAVE_SCK_GPIO_DEF(HS_SPI, B, 13, GPIO_AF_5);
SPI_SLAVE_MOSI_GPIO_DEF(HS_SPI, B, 15, GPIO_AF_5);
SPI_SLAVE_MISO_GPIO_DEF(HS_SPI, B, 14, GPIO_AF_5);
SPI_SLAVE_CS_GPIO_DEF(HS_SPI, B, 12, GPIO_AF_5, NULL);

SPI_TX_DMA_DEF(HS_SPI, SPI2, DMA0, DMA_CH2);
SPI_RX_DMA_DEF(HS_SPI, SPI2, DMA0, DMA_CH3);

/* v2.0：SPI_DEF 宏缺少 spi_control 与 4 个回调字段初始化，
 * 在 -Werror=missing-field-initializers 下无法编译，改为显式初始化 */
typdef_spi_struct HS_SPI = {
    RCU_SPI2, SPI2, SPI_SLAVE, SPI_DATASIZE_8BIT,
    SPI_CK_PL_LOW_PH_1EDGE, SPI_PSC_2, SPI_ENDIAN_MSB, MODE_DMA,
    &HS_SPI_SPI_SCK_GPIO, &HS_SPI_SPI_MOSI_GPIO,
    &HS_SPI_SPI_MISO_GPIO, &HS_SPI_SPI_CS_GPIO,
    &HS_SPI_SPI_TX_DMA, &HS_SPI_SPI_RX_DMA,
    {{0}, NULL, NULL, 0, 0, 0, 0}, NULL, NULL, NULL, NULL
};

#define SPI_LINK_RX_MAX  256

static uint8_t s_rx_buf[SPI_LINK_RX_MAX];    /* SPI 接收缓冲 */
static uint8_t s_tx_buf[SPI_LINK_RX_MAX];    /* SPI 发送缓冲（应答） */
static uint8_t s_frame_buf[SPI_LINK_RX_MAX]; /* 待发送帧（link 帧） */
static uint16_t s_frame_len = 0;
static volatile uint32_t s_rx_cnt = 0;
static bool s_inited = false;
static bool s_dma_armed = false;             /* RX DMA 是否已武装（常驻） */
static uint32_t s_rx_last_cnt = 0;           /* 上次观测到的已收字节数 */
static uint8_t  s_partial_wait = 0;          /* 部分接收计数稳定轮数 */

void spi_slave_link_init(void)
{
    driver_spi_init(&HS_SPI);
    /* 预置 TX 缓冲为 0xFF（空闲应答） */
    memset(s_tx_buf, 0xFF, sizeof(s_tx_buf));
    s_inited = true;
}

/* 重新武装 RX/TX DMA（非阻塞，不等待任何标志；调用方需保证上一次事务已结束） */
static void spi_slave_dma_arm(void)
{
    memset(s_rx_buf, 0, sizeof(s_rx_buf));

    /* 若有待发帧，装载到 TX 缓冲并在本次 SPI 事务中应答给主机 */
    if (s_frame_len > 0) {
        s_tx_buf[0] = (uint8_t)s_frame_len;
        memcpy(&s_tx_buf[1], s_frame_buf, s_frame_len);
        s_frame_len = 0;
    } else {
        s_tx_buf[0] = 0;   /* 无数据应答 */
    }

    spi_i2s_flag_clear(HS_SPI.spi_x,
                       SPI_STATC_TXURERRC | SPI_STATC_RXORERRC |
                       SPI_STATC_CRCERRC | SPI_STATC_FERRC | SPI_STATC_CONFERRC);
    spi_dma_disable(HS_SPI.spi_x, SPI_DMA_TRANSMIT);
    spi_dma_disable(HS_SPI.spi_x, SPI_DMA_RECEIVE);
    driver_dma_start(HS_SPI.spi_rx_dma, s_rx_buf, SPI_LINK_RX_MAX);
    driver_dma_start(HS_SPI.spi_tx_dma, s_tx_buf, SPI_LINK_RX_MAX);
    spi_dma_enable(HS_SPI.spi_x, SPI_DMA_TRANSMIT);
    spi_dma_enable(HS_SPI.spi_x, SPI_DMA_RECEIVE);
    s_dma_armed = true;
}

/* 处理一帧：首字节 = 数据长度，其余为 link 帧字节流 */
static void spi_slave_process_rx(uint16_t received)
{
    uint16_t len = s_rx_buf[0];   /* 首字节 = 数据长度 */
    if (received > 1 && len > 0 && len < SPI_LINK_RX_MAX && len < received) {
        /* DMA 写入后先失效 D-Cache 行，避免 CPU 读到陈旧缓存数据 */
        dcache_invalidate_region((uint32_t)&s_rx_buf[1], len);
        /* 转交 link 解析（与 UART3 同管线） */
        event_ipc_feed(&s_rx_buf[1], len);
        s_rx_cnt++;
    }
}

void spi_slave_link_poll(void)
{
    if (!s_inited) return;

    if (!s_dma_armed) {
        spi_slave_dma_arm();
        return;
    }

    /* 非阻塞查询 DMA 完成标志（主机完成一次 256B 全量事务） */
    if (dma_flag_get(HS_SPI_SPI_RX_DMA.dmax, HS_SPI_SPI_RX_DMA.dma_chx,
                     DMA_FLAG_FTF) == SET) {
        dma_flag_clear(HS_SPI_SPI_RX_DMA.dmax, HS_SPI_SPI_RX_DMA.dma_chx,
                       DMA_FLAG_FTF);
        spi_slave_process_rx(SPI_LINK_RX_MAX);
        spi_slave_dma_arm();          /* 立即重新武装，等待下一次事务 */
        s_rx_last_cnt = 0;
        s_partial_wait = 0;
        return;
    }

    /* 主机短事务场景（仅发送 n<256 字节）：DMA 永不完成，
     * 依据 DMA 剩余计数判断已收字节；连续两轮 poll 计数稳定
     * （间隔约 20ms）即认为事务已结束，按实际字节数处理。 */
    uint32_t received = SPI_LINK_RX_MAX - dma_transfer_number_get(
                            HS_SPI_SPI_RX_DMA.dmax, HS_SPI_SPI_RX_DMA.dma_chx);
    if (received >= 2 && received == s_rx_last_cnt) {
        if (++s_partial_wait >= 2) {
            spi_slave_process_rx((uint16_t)received);
            spi_slave_dma_arm();
            s_rx_last_cnt = 0;
            s_partial_wait = 0;
        }
    } else if (received != s_rx_last_cnt) {
        s_partial_wait = 0;           /* 仍在接收中，等待稳定 */
    }
    s_rx_last_cnt = received;
}

int spi_slave_link_send(const uint8_t *data, uint16_t len)
{
    if (!s_inited || data == NULL || len == 0 || len >= SPI_LINK_RX_MAX) {
        return -1;
    }
    if (s_frame_len != 0) return 1;   /* 上帧未发完：忙 */

    /* 缓冲待发帧（主机下一次 SPI 轮询时带出） */
    memcpy(s_frame_buf, data, len);
    s_frame_len = len;
    return 0;
}

uint32_t spi_slave_link_rx_count(void) { return s_rx_cnt; }
