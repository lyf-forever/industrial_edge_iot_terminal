#ifndef __EVENT_BUS_H_
#define __EVENT_BUS_H_

/**
 * @file event_bus.h
 * @brief 本地事件总线（GD32H7 端，架构 2.8 镜像）
 *
 * 与 ESP32S3 端 event_bus 保持相同 API 语义：
 * 订阅者数组 + 同步分发 + 临界区保护（GD32H7 为裸机轮询环境，
 * 无 RTOS 互斥锁，采用 __disable_irq/__enable_irq 临界区）。
 * 配合 event_ipc 实现跨核透明分发。
 */

#include <stdint.h>
#include <stdbool.h>
#include "event_id.h"

/* 事件回调原型（与 ESP32S3 端一致） */
typedef void (*event_cb_t)(uint16_t event_id, const uint8_t *payload,
                           uint8_t len, uint8_t source, void *user_data);

#define EVENT_PAYLOAD_MAX  9
#define EVT_REMOTE_WILDCARD  0xFFFE
#define EVENT_SUBSCRIBER_MAX  8

typedef int8_t event_sub_t;

/* ===================== API ===================== */

/* 初始化事件总线（main 中调用） */
void event_bus_init(void);

/* 订阅事件 */
event_sub_t event_bus_subscribe(uint16_t event_id, event_cb_t cb, void *user_data);

/* 订阅跨核通配（供 event_ipc 注册） */
event_sub_t event_bus_subscribe_remote(event_cb_t cb, void *user_data);

/* 退订 */
void event_bus_unsubscribe(event_sub_t sub);

/* 发布事件（local 分发；source=0 本地产生，1=远端转发） */
int event_bus_publish(uint16_t event_id, event_orient_t orient,
                      const uint8_t *payload, uint8_t len, uint8_t source);

/* 便捷发布：本地 + 跨核 */
static inline int event_bus_publish_any(uint16_t event_id,
                                        const uint8_t *payload, uint8_t len)
{
    return event_bus_publish(event_id, EVENT_ORIENT_ANY, payload, len, 0);
}

/* 仅本地发布 */
static inline int event_bus_publish_local(uint16_t event_id,
                                          const uint8_t *payload, uint8_t len)
{
    return event_bus_publish(event_id, EVENT_ORIENT_LOCAL, payload, len, 0);
}

#endif /* __EVENT_BUS_H_ */