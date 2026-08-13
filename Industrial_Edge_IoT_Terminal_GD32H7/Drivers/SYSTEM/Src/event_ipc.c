/**
 * @file event_ipc.c
 * @brief 跨核事件桥接实现（GD32H7 端）
 *
 * 数据流：
 *   TX: event_bus_publish_any(需跨核事件, source=0)
 *        -> 通配订阅回调 -> 编码 LINK_CMD_EVENT 帧 -> bt_send_dma 发送
 *   RX: UART3 收到帧 -> link_parse_buffer -> 帧回调
 *        -> LINK_CMD_EVENT 解码 -> event_bus_publish(source=1, local)
 */
#include "event_ipc.h"
#include "event_bus.h"
#include "event_id.h"
#include "link_protocol.h"
#include "BT24.h"
#include <string.h>

static event_sub_t s_sub_remote = -1;
static uint32_t s_tx_cnt = 0;
static uint32_t s_rx_cnt = 0;

/* 本端地址（与 ESP32S3 端 LINK_ADDR_GD32H7 对应） */
#define GD32_LINK_ADDR  0x02

/* 解析上下文（由外部 main 提供或本文件静态持有） */
static link_parse_ctx_t s_link_ctx;

/* ===================== TX：总线通配 -> link 帧 ===================== */
static void on_event_for_remote(uint16_t event_id, const uint8_t *payload,
                                uint8_t len, uint8_t source, void *user_data)
{
    (void)user_data;
    if (source != 0) return;                       /* 远端转发来的不回发 */
    if (!EVENT_IS_REMOTE(event_id)) return;        /* 仅需跨核的事件 */

    /* 事件帧载荷：event_id(2) + source(1) + payload(9) = 12 */
    uint8_t data[LINK_MAX_DATA_LEN] = {0};
    data[0] = (uint8_t)(event_id & 0xFF);
    data[1] = (uint8_t)(event_id >> 8);
    data[2] = 0;                                   /* source=本地产生 */
    if (payload != NULL && len > 0) {
        uint8_t cp = len > 9 ? 9 : len;
        memcpy(&data[3], payload, cp);
    }

    uint8_t frame[40];
    uint16_t flen = build_frame(GD32_LINK_ADDR, LINK_CMD_EVENT,
                                data, LINK_MAX_DATA_LEN, frame, sizeof(frame));
    if (flen > 0) {
        /* 架构 3.3b：双链路冗余——UART 主链路发送 + SPI 高速链路缓冲 */
        bool sent = false;
        if (bt_send_dma(frame, flen) == 0) {
            sent = true;
        }
        extern int spi_slave_link_send(const uint8_t *, uint16_t);
        if (spi_slave_link_send(frame, flen) == 0) {
            sent = true;
        }
        if (sent) s_tx_cnt++;
    }
}

/* ===================== RX：link 帧 -> 本地总线 ===================== */
static bool on_link_frame(uint8_t addr, uint8_t cmd,
                          const uint8_t *data, uint8_t data_len, void *user_data)
{
    (void)addr;
    (void)user_data;

    if (cmd == LINK_CMD_EVENT && data_len >= LINK_MAX_DATA_LEN) {
        uint16_t event_id = (uint16_t)(data[0] | (data[1] << 8));
        /* 投递本地总线(source=1 远端，仅本地分发) */
        event_bus_publish(event_id, EVENT_ORIENT_LOCAL, &data[3], 9, 1);
        s_rx_cnt++;
        return true;
    }

    /* 兼容旧版直接命令帧：转译为事件。
     * 注意：这些事件源自远端（source 必须为 1），若用 source=0 发布，
     * 通配订阅回调（on_event_for_remote）会将其重新编码回发远端，形成回环。 */
    switch (cmd) {
        case LINK_CMD_SENSOR_DATA:
            event_bus_publish(EVT_SENSOR_DATA, EVENT_ORIENT_LOCAL, data, data_len, 1);
            return true;
        case LINK_CMD_ALARM:
            event_bus_publish(EVT_SENSOR_ALARM, EVENT_ORIENT_LOCAL, data, data_len, 1);
            return true;
        case LINK_CMD_LED_CTRL:
            event_bus_publish(EVT_CTRL_LED, EVENT_ORIENT_LOCAL, data, data_len, 1);
            return true;
        case LINK_CMD_HEARTBEAT:
            event_bus_publish(EVT_SYS_HEARTBEAT, EVENT_ORIENT_LOCAL, NULL, 0, 1);
            return true;
        case LINK_CMD_REBOOT:
            event_bus_publish(EVT_SYS_REBOOT, EVENT_ORIENT_LOCAL, NULL, 0, 1);
            return true;
        case LINK_CMD_TIME_SYNC:
            /* v2.0 补齐：时间同步帧接入 EVT_SYS_TIME_SYNC 事件（此前落入 default 丢弃） */
            event_bus_publish(EVT_SYS_TIME_SYNC, EVENT_ORIENT_LOCAL, data, data_len, 1);
            return true;
        default:
            break;
    }
    return false;
}

void event_ipc_init(void)
{
    link_parse_init(&s_link_ctx);
    s_sub_remote = event_bus_subscribe_remote(on_event_for_remote, NULL);
    link_set_frame_callback(on_link_frame, NULL);
}

void event_ipc_feed(const uint8_t *data, uint16_t len)
{
    if (data == NULL) return;
    link_parse_buffer(&s_link_ctx, data, len);
}

uint32_t event_ipc_tx_count(void) { return s_tx_cnt; }
uint32_t event_ipc_rx_count(void) { return s_rx_cnt; }