/**
 * @file event_bus.c
 * @brief 本地事件总线实现（GD32H7 端）
 *
 * 订阅者数组 + 同步分发，临界区（关中断）保护并发。
 * 跨核转发由 event_ipc 通过订阅 EVT_REMOTE_WILDCARD 完成。
 */
#include "event_bus.h"
#include "gd32h7xx.h"   /* __disable_irq/__enable_irq (CMSIS) */
#include <string.h>

/* CMSIS 临界区宏：保存/恢复 PRIMASK，避免在外部临界区（如 DMA 双缓冲切换）
 * 内发布事件时提前使能中断 */
static inline uint32_t critical_enter(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static inline void critical_exit(uint32_t primask)
{
    __set_PRIMASK(primask);
}

typedef struct {
    bool       used;
    uint16_t   event_id;
    event_cb_t cb;
    void      *user_data;
} event_subscriber_t;

static event_subscriber_t s_subs[EVENT_SUBSCRIBER_MAX];
static bool s_inited = false;

void event_bus_init(void)
{
    memset(s_subs, 0, sizeof(s_subs));
    s_inited = true;
}

event_sub_t event_bus_subscribe(uint16_t event_id, event_cb_t cb, void *user_data)
{
    if (!s_inited || cb == NULL) return -1;
    event_sub_t sub = -1;
    uint32_t primask = critical_enter();
    for (int i = 0; i < EVENT_SUBSCRIBER_MAX; i++) {
        if (!s_subs[i].used) {
            s_subs[i].used = true;
            s_subs[i].event_id = event_id;
            s_subs[i].cb = cb;
            s_subs[i].user_data = user_data;
            sub = (event_sub_t)i;
            break;
        }
    }
    critical_exit(primask);
    return sub;
}

event_sub_t event_bus_subscribe_remote(event_cb_t cb, void *user_data)
{
    return event_bus_subscribe(EVT_REMOTE_WILDCARD, cb, user_data);
}

void event_bus_unsubscribe(event_sub_t sub)
{
    if (sub < 0 || sub >= EVENT_SUBSCRIBER_MAX) return;
    uint32_t primask = critical_enter();
    s_subs[sub].used = false;
    s_subs[sub].cb = NULL;
    critical_exit(primask);
}

int event_bus_publish(uint16_t event_id, event_orient_t orient,
                      const uint8_t *payload, uint8_t len, uint8_t source)
{
    (void)orient;   /* GD32H7 端为消息源端：跨核由 event_ipc 通配订阅处理 */
    if (!s_inited) return -1;
    if (len > EVENT_PAYLOAD_MAX) len = EVENT_PAYLOAD_MAX;

    int dispatched = 0;
    uint32_t primask = critical_enter();
    for (int i = 0; i < EVENT_SUBSCRIBER_MAX; i++) {
        if (!s_subs[i].used || s_subs[i].cb == NULL) continue;
        if (s_subs[i].event_id == event_id ||
            s_subs[i].event_id == EVT_REMOTE_WILDCARD) {
            s_subs[i].cb(event_id, payload, len, source, s_subs[i].user_data);
            dispatched++;
        }
    }
    critical_exit(primask);
    return dispatched;
}