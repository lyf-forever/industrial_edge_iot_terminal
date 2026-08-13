/**
 * @file channel_spi.c
 * @brief HS-SPI 通道实现（ESP-IDF SPI master 封装为 channel_t）
 *
 * 注意：本实现为可用的 SPI master 通道框架，接入 channel 表后
 * 即可与 UART 通道并行注册、由 event_ipc 双链路热切换。
 * 实际联调需对端(GD32H7) SPI 从机驱动配合（文档 Phase 4）。
 */

#include "channel_spi.h"
#include "channel.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include <string.h>

#if ChannelUse

static const char *TAG = "ch_spi";

static spi_device_handle_t s_spi_dev = NULL;
static bool s_inited = false;

/* 接收回调（由 channel 层设置） */
static channel_rx_cb_t s_rx_cb = NULL;
static void *s_rx_user = NULL;

static int ch_spi_send(const uint8_t *d, uint16_t n)
{
    if (!s_inited || d == NULL || n == 0) return -1;
    /* 半双工主机发送（发送时无接收数据） */
    spi_transaction_t t = {0};
    t.length = n * 8;
    t.tx_buffer = d;
    esp_err_t err = spi_device_transmit(s_spi_dev, &t);
    return (err == ESP_OK) ? (int)n : -1;
}

static void ch_spi_set_rx_cb(channel_rx_cb_t cb, void *user)
{
    s_rx_cb = cb;
    s_rx_user = user;
}

static uint8_t ch_spi_link_up(void)
{
    return s_inited ? 1 : 0;
}

static void ch_spi_deinit(void)
{
    if (s_spi_dev) {
        spi_bus_remove_device(s_spi_dev);
        s_spi_dev = NULL;
    }
}

static const channel_t s_spi_channel = {
    .name = "hs-spi",
    .send = ch_spi_send,
    .set_rx_cb = ch_spi_set_rx_cb,
    .link_up = ch_spi_link_up,
    .deinit = ch_spi_deinit,
};

/* SPI 接收轮询：由调用方周期调用（或独立任务），
 * 向从机发起读请求并接收应答数据。 */
void channel_spi_poll_rx(void)
{
    if (!s_inited || s_rx_cb == NULL) return;
    uint8_t buf[256];
    spi_transaction_t t = {0};
    t.length = 256 * 8;
    t.rx_buffer = buf;
    if (spi_device_transmit(s_spi_dev, &t) == ESP_OK) {
        /* 假设从机在第一个字节返回有效长度（示例协议：len + data） */
        uint16_t len = buf[0];
        if (len > 0 && len <= 256 && len < 0xE0) {
            s_rx_cb(&buf[1], len, s_rx_user);
        }
    }
}

void channel_spi_register(void)
{
    if (s_inited) return;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = CH_SPI_MOSI_GPIO,
        .miso_io_num = CH_SPI_MISO_GPIO,
        .sclk_io_num = CH_SPI_SCK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
        return;
    }

    spi_device_interface_config_t dev_cfg = {
        .mode = 0,
        .clock_speed_hz = CH_SPI_CLK_HZ,
        .spics_io_num = CH_SPI_CS_GPIO,
        .queue_size = 4,
    };
    err = spi_bus_add_device(SPI2_HOST, &dev_cfg, &s_spi_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(err));
        return;
    }

    s_inited = true;
    channel_register(&s_spi_channel);
    ESP_LOGI(TAG, "hs-spi channel registered (clk=%d)", CH_SPI_CLK_HZ);
}

#endif /* ChannelUse */