/**
 * @file event_ipc.c
 * @brief 跨核事件桥接实现
 *
 * 同时是：本地事件总线订阅者(通配) + link_protocol 帧接收者。
 * 单向数据流：
 *   本地 publish(orient=ANY/REMOTE, source=0)
 *     -> event_ipc 收到通配回调
 *     -> encode LINK_CMD_EVENT -> bsp_uart -> 对端(GD32H7)
 *   对端发来 LINK_CMD_EVENT 帧
 *     -> link_protocol 帧回调(本模块注册)
 *     -> decode event -> event_bus_publish(source=REMOTE, local only)
 */

#include "event_ipc.h"

#if EventBusUse && LinkUse

#include "event_bus.h"
#include "event_id.h"
#include "link_protocol.h"
#include "link_payload.h"
#include "bsp_uart.h"
#if BleGattUse || TcpSrvUse
#include "net_bridge.h"
#endif
#if ChannelUse
#include "channel.h"
#endif
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "event_ipc";

static event_sub_t s_sub_remote = -1;
static uint32_t s_tx_cnt = 0;
static uint32_t s_rx_cnt = 0;
static link_parse_ctx_t s_link_ctx;   /* link 帧解析上下文 */

/* 跨核链路状态（供 conn_hsm 订阅），默认 DOWN，link 收到帧即 UP */
static uint32_t s_last_rx_tick = 0;
#define LINK_TIMEOUT_MS  30000

/* ===================== TX：总线通配订阅回调 -> link 帧 ============== *
 * 对所有需跨核转发的事件(EVENT_IS_REMOTE)且来源为本地(source==0)的事件，
 * 编码为 LINK_CMD_EVENT 帧发往对端。来源==REMOTE 的不回发(防环)。
 * */
static void on_event_for_remote(uint16_t event_id, const uint8_t *payload,
                                uint8_t len, uint8_t source, void *user_data)
{
    (void)user_data;
    if (source != 0) return;                  /* 远端转发来的不再回发 */
    if (!EVENT_IS_REMOTE(event_id)) {         /* 仅传感器/控制/系统类跨核 */
        return;
    }

    event_payload_t ep = {0};
    ep.event_id = event_id;
    ep.source   = 0;                          /* 标记本地产生 */
    if (payload != NULL && len > 0) {
        uint8_t cp = len > 9 ? 9 : len;
        memcpy(ep.payload, payload, cp);
    }

    uint8_t data[LINK_MAX_DATA_LEN];
    link_payload_event_pack(&ep, data);

    uint8_t frame[40];
    uint16_t flen = build_frame(LINK_ADDR_ESP32S3, LINK_CMD_EVENT,
                                data, LINK_MAX_DATA_LEN,
                                frame, sizeof(frame));
    if (flen > 0) {
#if ChannelUse
        /* 架构 3.3：经通道抽象发送（可热切换 UART/SPI） */
        channel_send(frame, flen);
#else
        bsp_uart_send(frame, flen);
#endif
#if BleGattUse || TcpSrvUse
        /* 无线透传桥镜像广播：BLE notify + TCP 客户端（跨核事件对 APP 可见） */
        net_bridge_broadcast(frame, flen);
#endif
        s_tx_cnt++;
    } else {
        ESP_LOGW(TAG, "build_frame failed evt=0x%04X", event_id);
    }
}

/* ===================== RX：UART 字节流 -> link 解析 ================== *
 * bsp_uart 收到字节流后回调，喂入 link_parse 状态机，解析到完整帧再调用
 * on_link_frame -> 本地总线投递。
 * */
static void on_uart_rx(const uint8_t *data, uint16_t len, void *user_data)
{
    link_parse_ctx_t *ctx = (link_parse_ctx_t *)user_data;
    if (ctx == NULL) return;
    link_parse_buffer(ctx, data, len);
}

/* ===================== RX：link 帧回调 -> 本地总线投递 ============== *
 * 仅处理 LINK_CMD_EVENT 帧，解码事件并以 source=REMOTE 本地分发。
 * 其余命令帧交给 cloud_bridge 等业务订阅者处理(通过总线发布对应事件)。
 * */
static bool on_link_frame(uint8_t addr, uint8_t cmd,
                          const uint8_t *data, uint8_t data_len, void *user_data)
{
    (void)addr;
    (void)user_data;

    s_last_rx_tick = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (cmd == LINK_CMD_EVENT && data_len >= LINK_MAX_DATA_LEN) {
        event_payload_t ep;
        link_payload_event_unpack(data, &ep);
        /* 投递到本地总线(source=1 远端)，仅本地分发(防环) */
        event_bus_publish(ep.event_id, EVENT_ORIENT_LOCAL,
                          ep.payload, sizeof(ep.payload), 1);
#if TopicUse
        /* 架构 3.2：主题化订阅——同时向字符串主题订阅者分发 */
        extern bool topic_dispatch(uint16_t, const uint8_t *, uint8_t, uint8_t, void *);
        topic_dispatch(ep.event_id, ep.payload, sizeof(ep.payload), 1, NULL);
#endif
        s_rx_cnt++;
        ESP_LOGI(TAG, "RX evt=0x%04X from remote", ep.event_id);
        return true;
    }

    /* 兼容旧版直接命令帧：转译为事件，供总线订阅者统一处理 */
    switch (cmd) {
        case LINK_CMD_SENSOR_DATA: {
            event_bus_publish_local(EVT_SENSOR_DATA, data, data_len);
            return true;
        }
        case LINK_CMD_ALARM: {
            event_bus_publish_local(EVT_SENSOR_ALARM, data, data_len);
            return true;
        }
        case LINK_CMD_LED_CTRL: {
            event_bus_publish_local(EVT_CTRL_LED, data, data_len);
            return true;
        }
        case LINK_CMD_HEARTBEAT: {
            event_bus_publish_local(EVT_SYS_HEARTBEAT, NULL, 0);
            return true;
        }
        case LINK_CMD_REBOOT: {
            event_bus_publish_local(EVT_SYS_REBOOT, NULL, 0);
            return true;
        }
        default:
            break;
    }
    return false;
}

void event_ipc_init(void *arg)
{
    (void)arg;
    /* 接管 RX -> link 解析链路：收到的字节流喂入解析器，
     * 解析到完整帧后调用 on_link_frame -> 本地总线投递 */
    link_parse_init(&s_link_ctx);
#if ChannelUse
    /* 架构 3.3：经通道抽象设置 RX 回调（活动通道） */
    channel_set_rx_cb(on_uart_rx, &s_link_ctx);
#else
    bsp_uart_set_rx_callback(on_uart_rx, &s_link_ctx);
#endif
    /* 注册为总线通配订阅者，接管所有总线上需跨核转发的事件 */
    s_sub_remote = event_bus_subscribe_remote(on_event_for_remote, NULL);
    /* 注册 link 帧回调，收到的跨核事件经解码投递本地总线 */
    link_set_frame_callback(on_link_frame, NULL);
    ESP_LOGI(TAG, "event ipc bridge init ok (sub=%d)", s_sub_remote);
}

void event_ipc_task(void *arg)
{
    (void)arg;
    const TickType_t period = pdMS_TO_TICKS(5000);
    uint32_t hb_cnt = 0;
    while (1) {
        vTaskDelay(period);
        hb_cnt++;
        /* 周期心跳事件，经总线驱动跨核心跳 */
        event_bus_publish_any(EVT_SYS_HEARTBEAT, NULL, 0);

        /* 链路健康检测：超过阈值未收帧，发布链路 DOWN 事件
         * 并触发双链路热切换（主通道超时 -> 备用通道） */
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (s_last_rx_tick != 0 && (now - s_last_rx_tick) > LINK_TIMEOUT_MS) {
            comm_state_payload_t cs = { .link_id = 2, .state = COMM_STATE_DOWN };
            event_bus_publish_local(EVT_COMM_LINK_STATE, (uint8_t *)&cs, sizeof(cs));
#if ChannelUse
            /* 架构 3.3：热切换——活动通道超时则切到下一通道（UART<->SPI 双链路冗余）
             * v2.0：通道数动态查询，单链路配置（仅 UART）时不再尝试切换不存在的通道 */
            int8_t active = channel_active_idx();
            if (active >= 0) {
                int8_t n = channel_count();
                if (n > 1) {
                    int8_t next = (active + 1) % n;
                    if (next != active) {
                        ESP_LOGW(TAG, "link timeout, switching channel %d -> %d",
                                 active, next);
                        channel_select(next);
                    }
                } else {
                    ESP_LOGW(TAG, "link timeout, only 1 channel registered, no switch");
                }
            }
#endif
        } else if (s_last_rx_tick != 0) {
            static bool up_reported = false;
            if (!up_reported) {
                comm_state_payload_t cs = { .link_id = 2, .state = COMM_STATE_UP };
                event_bus_publish_local(EVT_COMM_LINK_STATE, (uint8_t *)&cs, sizeof(cs));
                up_reported = true;
            }
        }
    }
}

uint32_t event_ipc_tx_count(void) { return s_tx_cnt; }
uint32_t event_ipc_rx_count(void) { return s_rx_cnt; }

/* 外部字节流（BLE/TCP 透传桥）喂入同一 link 解析管线 */
void event_ipc_feed_rx(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) return;
    link_parse_buffer(&s_link_ctx, data, len);
}

#endif /* EventBusUse && LinkUse */