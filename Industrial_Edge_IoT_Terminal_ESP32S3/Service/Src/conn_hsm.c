/**
 * @file conn_hsm.c
 * @brief 连接层次状态机实现
 *
 * 订阅事件总线上的 EVT_COMM_* 状态事件，维护层次状态：
 *   OFFLINE <-> CLOUD_DISCONNECTED <-> CLOUD_CONNECTED
 * 跃迁时发布 HSM 状态变化事件（供显示/上报订阅），并直接驱动 WS2812
 * 状态指示灯（绿/黄/红三态），实现"一盏灯反映全链路连接健康度"。
 */

#include "conn_hsm.h"

#if HsmUse

#include "event_bus.h"
#include "event_id.h"
#include "esp_log.h"
#if Ws2812Use
#include "bsp_ws2812.h"
#endif

static const char *TAG = "conn_hsm";

static hsm_state_t s_state = HSM_STATE_OFFLINE;
static bool s_wifi_up = false;
static bool s_mqtt_up = false;
static bool s_link_up = false;

static void update_indication(void)
{
#if Ws2812Use
    switch (s_state) {
        case HSM_STATE_CLOUD_CONNECTED:
            ws2812_writeGRB(0, 12, 0);   /* 绿色：全链路就绪 */
            break;
        case HSM_STATE_CLOUD_DISCONNECTED:
            ws2812_writeGRB(12, 12, 0);  /* 黄色：链路通但未上云 */
            break;
        case HSM_STATE_OFFLINE:
        default:
            ws2812_writeGRB(18, 0, 0);   /* 红色：跨核链路断 */
            break;
    }
#endif
}

static void recompute_state(void)
{
    hsm_state_t next;
    if (!s_link_up) {
        next = HSM_STATE_OFFLINE;
    } else if (s_wifi_up && s_mqtt_up) {
        next = HSM_STATE_CLOUD_CONNECTED;
    } else {
        next = HSM_STATE_CLOUD_DISCONNECTED;
    }

    if (next != s_state) {
        ESP_LOGI(TAG, "HSM transition: %d -> %d", s_state, next);
        s_state = next;
        update_indication();
        /* 发布状态变化事件，供显示/上报订阅 */
        uint8_t st = (uint8_t)s_state;
        event_bus_publish_local(EVT_SYS_STATUS, &st, 1);
    }
}

/* 通信状态事件回调 */
static void on_comm_state(uint16_t event_id, const uint8_t *payload,
                          uint8_t len, uint8_t source, void *user_data)
{
    (void)source;
    (void)user_data;
    if (payload == NULL || len < 2) return;

    uint8_t link_id = payload[0];
    uint8_t state   = payload[1];

    switch (link_id) {
        case 0: /* Wi-Fi */
            s_wifi_up = (state == COMM_STATE_UP);
            break;
        case 1: /* MQTT */
            s_mqtt_up = (state == COMM_STATE_UP);
            break;
        case 2: /* Link */
            s_link_up = (state == COMM_STATE_UP);
            break;
        default:
            break;
    }
    recompute_state();
}

void conn_hsm_init(void *arg)
{
    (void)arg;
    event_bus_subscribe(EVT_COMM_WIFI_STATE, on_comm_state, NULL);
    event_bus_subscribe(EVT_COMM_MQTT_STATE, on_comm_state, NULL);
    event_bus_subscribe(EVT_COMM_LINK_STATE, on_comm_state, NULL);
    update_indication();
    ESP_LOGI(TAG, "conn HSM init ok (state=%d)", s_state);
}

hsm_state_t conn_hsm_get_state(void)
{
    return s_state;
}

#endif /* HsmUse */