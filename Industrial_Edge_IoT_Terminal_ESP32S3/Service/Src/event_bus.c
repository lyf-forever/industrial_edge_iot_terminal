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
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "event_bus";

typedef struct {
    bool      used;
    uint16_t  event_id;     /* EVT_REMOTE_WILDCARD 表示跨核通配订阅 */
    event_cb_t cb;
    void     *user_data;
} event_subscriber_t;

/* ISR 延迟发布：ISR 入队，bridge 任务出队后走常规 publish（v2.0） */
typedef struct {
    uint16_t event_id;
    uint8_t  payload[EVENT_PAYLOAD_MAX];
    uint8_t  len;
} isr_pending_t;

#define EVENT_BUS_ISR_Q_DEPTH  8

static event_subscriber_t s_subs[EVENT_SUBSCRIBER_MAX];
static SemaphoreHandle_t  s_mtx = NULL;
static QueueHandle_t      s_isr_q = NULL;
static volatile bool      s_in_dispatch = false;  /* 回调内重入检测 */

/* ISR 事件桥接任务：出队 -> 常规发布（本地分发 + 跨核转发） */
static void event_bus_isr_bridge_task(void *arg)
{
    (void)arg;
    isr_pending_t m;
    while (1) {
        if (xQueueReceive(s_isr_q, &m, portMAX_DELAY) == pdTRUE) {
            event_bus_publish(m.event_id, EVENT_ORIENT_ANY, m.payload, m.len, 0);
        }
    }
}

void event_bus_init(void *arg)
{
    (void)arg;
    memset(s_subs, 0, sizeof(s_subs));
    s_mtx = xSemaphoreCreateMutex();
    s_isr_q = xQueueCreate(EVENT_BUS_ISR_Q_DEPTH, sizeof(isr_pending_t));
    if (s_isr_q != NULL) {
        BaseType_t ret = xTaskCreate(event_bus_isr_bridge_task, "evtIsrBridge",
                                     2048, NULL, 2, NULL);
        if (ret != pdPASS) {
            ESP_LOGW(TAG, "isr bridge task create failed");
        }
    }
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

    /* v2.0：回调内重入检测——订阅者回调中再次 publish 会死锁于普通 Mutex，
     * 此处拒绝并告警（编译期无法约束，运行时防护） */
    if (s_in_dispatch) {
        ESP_LOGW(TAG, "publish re-entered evt=0x%04X from callback, rejected "
                 "(recursive publish not allowed)", event_id);
        return 0;
    }

    int dispatched = 0;

    /* 本地分发：LOCAL 或 ANY */
    if (orient & EVENT_ORIENT_LOCAL) {
        xSemaphoreTake(s_mtx, portMAX_DELAY);
        s_in_dispatch = true;
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
        s_in_dispatch = false;
        xSemaphoreGive(s_mtx);
    }
    return dispatched;
}

int event_bus_publish_isr(uint16_t event_id, const uint8_t *payload, uint8_t len)
{
    if (s_isr_q == NULL) return -1;
    if (len > EVENT_PAYLOAD_MAX) len = EVENT_PAYLOAD_MAX;

    isr_pending_t m = { .event_id = event_id, .len = len };
    if (payload != NULL && len > 0) {
        memcpy(m.payload, payload, len);
    }
    BaseType_t woken = pdFALSE;
    if (xQueueSendFromISR(s_isr_q, &m, &woken) != pdTRUE) {
        return -1;   /* 队列满：丢弃，避免 ISR 阻塞 */
    }
    portYIELD_FROM_ISR(woken);
    return 0;
}

#endif /* EventBusUse */