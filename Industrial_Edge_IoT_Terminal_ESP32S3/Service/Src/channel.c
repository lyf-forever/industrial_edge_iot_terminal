/**
 * @file channel.c
 * @brief 通道抽象实现：注册表 + 活动通道热切换 + UART 通道封装
 */

#include "channel.h"

#if ChannelUse

#include "bsp_uart.h"
#include "esp_log.h"
#include <string.h>

#define CHANNEL_MAX  4

static const channel_t *s_channels[CHANNEL_MAX];
static int8_t s_count = 0;
static int8_t s_active = -1;
static uint32_t s_switch_cnt = 0;

/* ===================== UART 通道封装 ===================== */
#if LinkUse
static int ch_uart_send(const uint8_t *d, uint16_t n) { return bsp_uart_send(d, n); }
static void ch_uart_set_rx_cb(channel_rx_cb_t cb, void *user)
{
    bsp_uart_set_rx_callback((void (*)(const uint8_t *, uint16_t, void *))cb, user);
}
static uint8_t ch_uart_link_up(void) { return 1; }   /* UART 驱动无显式链路状态，默认在线 */
static void ch_uart_deinit(void) {}

static const channel_t s_uart_channel = {
    .name = "uart",
    .send = ch_uart_send,
    .set_rx_cb = ch_uart_set_rx_cb,
    .link_up = ch_uart_link_up,
    .deinit = ch_uart_deinit,
};

void channel_uart_register(void)
{
    channel_register(&s_uart_channel);
    ESP_LOGI("channel", "uart channel registered");
}
#else
void channel_uart_register(void) {}
#endif /* LinkUse */

/* ===================== SPI 占位通道 ===================== */
static int ch_spi_send(const uint8_t *d, uint16_t n) { (void)d; (void)n; return -1; }
static void ch_spi_set_rx_cb(channel_rx_cb_t cb, void *user) { (void)cb; (void)user; }
static uint8_t ch_spi_link_up(void) { return 0; }
static void ch_spi_deinit(void) {}

static const channel_t s_spi_stub_channel = {
    .name = "spi(stub)",
    .send = ch_spi_send,
    .set_rx_cb = ch_spi_set_rx_cb,
    .link_up = ch_spi_link_up,
    .deinit = ch_spi_deinit,
};

void channel_spi_register_stub(void)
{
    channel_register(&s_spi_stub_channel);
    ESP_LOGI("channel", "spi stub registered");
}

/* ===================== 注册表 ===================== */
int8_t channel_register(const channel_t *ch)
{
    if (ch == NULL || s_count >= CHANNEL_MAX) return -1;
    s_channels[s_count] = ch;
    if (s_active < 0) s_active = s_count;   /* 首个注册者自动为活动通道 */
    return s_count++;
}

bool channel_select(int8_t idx)
{
    if (idx < 0 || idx >= s_count) return false;
    if (idx == s_active) return true;
    s_active = idx;
    s_switch_cnt++;
    ESP_LOGI("channel", "switch -> %s", s_channels[idx]->name);
    return true;
}

const channel_t *channel_active(void)
{
    if (s_active < 0) return NULL;
    return s_channels[s_active];
}

int8_t channel_active_idx(void) { return s_active; }

int8_t channel_count(void) { return s_count; }

int channel_send(const uint8_t *d, uint16_t n)
{
    const channel_t *ch = channel_active();
    if (ch == NULL || ch->send == NULL) return -1;
    return ch->send(d, n);
}

void channel_set_rx_cb(channel_rx_cb_t cb, void *user)
{
    const channel_t *ch = channel_active();
    if (ch == NULL || ch->set_rx_cb == NULL) return;
    ch->set_rx_cb(cb, user);
}

bool channel_is_up(void)
{
    const channel_t *ch = channel_active();
    if (ch == NULL || ch->link_up == NULL) return false;
    return ch->link_up() != 0;
}

uint32_t channel_switch_count(void) { return s_switch_cnt; }

/* init 表接入：注册内置通道（UART 主链路 + SPI 高速链路） */
void channel_init(void *arg)
{
    (void)arg;
    channel_uart_register();
#if ChannelSpiUse
    extern void channel_spi_register(void);
    channel_spi_register();
#else
    channel_spi_register_stub();
#endif
}

#endif /* ChannelUse */