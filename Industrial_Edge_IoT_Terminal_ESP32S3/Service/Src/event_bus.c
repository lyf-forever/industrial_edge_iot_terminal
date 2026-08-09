/**
 * @file event_bus.c
 * @brief 本地事件总线实现
 *
 * 订阅者数组 + 发布时同步遍历调用。跨核转发由 event_ipc 通过订阅
 * EVT_REMOTE_WILDCARD 完成；本模块对"远端"概念无感知，仅按 source
 * 字段透传给订阅者，由 event_ipc 决定是否回发对端（防环）。
 */

#include "event_bus.h"

#if EventBusUse

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "event_bus";

typedef struct {
    bool      used;
    uint16_t  event_id;     /* EVT_REMOTE_WILDCARD 表示跨核通配订阅 */
    event_cb_t cb;
    void     *user_data;
} event_subscriber_t;

static event_subscriber_t s_subs[EVENT_SUBSCRIBER_MAX];
static SemaphoreHandle_t  s_mtx = NULL;

void event_bus_init(void *arg)
{
    (void)arg;
    memset(s_subs, 0, sizeof(s_subs));
    s_mtx = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "event bus init ok (cap=%d)", EVENT_SUBSCRIBER_MAX);
}

static event_sub_t find_free_slot(void)
{
    for (int i = 0; i < EVENT_SUBSCRIBER_MAX; i++) {
        if (!s_subs[i].used) return (event_sub_t)i;
    }
    return -1;
}

event_sub_t event_bus_subscribe(uint16_t event_id, event_cb_t cb, void *user_data)
{
    if (cb == NULL || s_mtx == NULL) return -1;
    event_sub_t sub = -1;
    xSemaphoreTake(s_mtx, portMAX_DELAY);
    sub = find_free_slot();
    if (sub >= 0) {
        s_subs[sub].used = true;
        s_subs[sub].event_id = event_id;
        s_subs[sub].cb = cb;
        s_subs[sub].user_data = user_data;
    }
    xSemaphoreGive(s_mtx);
    if (sub < 0) ESP_LOGW(TAG, "subscribe full: evt=0x%04X", event_id);
    return sub;
}

event_sub_t event_bus_subscribe_remote(event_cb_t cb, void *user_data)
{
    return event_bus_subscribe(EVT_REMOTE_WILDCARD, cb, user_data);
}

void event_bus_unsubscribe(event_sub_t sub)
{
    if (sub < 0 || sub >= EVENT_SUBSCRIBER_MAX || s_mtx == NULL) return;
    xSemaphoreTake(s_mtx, portMAX_DELAY);
    s_subs[sub].used = false;
    s_subs[sub].cb = NULL;
    xSemaphoreGive(s_mtx);
}

int event_bus_publish(uint16_t event_id, event_orient_t orient,
                      const uint8_t *payload, uint8_t len, uint8_t source)
{
    (void)len;
    if (s_mtx == NULL) return -1;
    if (len > EVENT_PAYLOAD_MAX) len = EVENT_PAYLOAD_MAX;

    int dispatched = 0;

    /* 本地分发：LOCAL 或 ANY */
    if (orient & EVENT_ORIENT_LOCAL) {
        xSemaphoreTake(s_mtx, portMAX_DELAY);
        for (int i = 0; i < EVENT_SUBSCRIBER_MAX; i++) {
            if (!s_subs[i].used || s_subs[i].cb == NULL) continue;
            /* 精确匹配 或 跨核通配订阅 */
            if (s_subs[i].event_id == event_id ||
                s_subs[i].event_id == EVT_REMOTE_WILDCARD) {
                /* 通配订阅者(event_ipc)负责跨核转发逻辑，
                 * 精确订阅者负责业务处理。此处统一回调。 */
                s_subs[i].cb(event_id, payload, len, source, s_subs[i].user_data);
                dispatched++;
            }
        }
        xSemaphoreGive(s_mtx);
    }
    return dispatched;
}

#endif /* EventBusUse */