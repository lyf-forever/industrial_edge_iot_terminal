#ifndef __EVENT_BUS_H_
#define __EVENT_BUS_H_

/**
 * @file event_bus.h
 * @brief 本地事件总线（发布/订阅消息中枢）
 *
 * 文档架构："以 Event Bus 作为系统级消息中枢，模块订阅、发布事件时
 * 无需关心事件由哪颗芯片处理"。本模块实现 ESP32S3 端的本地总线，
 * 配合 event_ipc 实现跨核透明分发。
 *
 * 设计要点：
 *  - 订阅者注册 (event_id, callback)；总线按 event_id 精确分发；
 *  - 发布时同步调用匹配订阅者回调（轻量、零队列、低延迟）；
 *  - 跨核转发由 event_ipc 作为特殊订阅者接管（订阅 EVT_REMOTE_WILDCARD）；
 *  - 防环：跨核转发的回调携带 source=REMOTE，本地发布时不再回发。
 *
 * 线程安全：发布/订阅均持互斥锁。回调内禁止阻塞/再次发布同事件(可重入
 * 但不推荐)。中断上下文请使用 event_bus_publish_isr。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"
#include "event_id.h"

#if EventBusUse

/* 事件回调原型
 * @param event_id  事件 ID
 * @param payload   事件附加负载指针（指向订阅者共享的只读缓冲，回调内勿持久保存）
 * @param len       负载字节数（0..EVENT_PAYLOAD_MAX）
 * @param source    事件来源：0=本地 1=远端转发
 * @param user_data 订阅时注册的私有上下文
 */
typedef void (*event_cb_t)(uint16_t event_id, const uint8_t *payload,
                           uint8_t len, uint8_t source, void *user_data);

/* 事件附加负载最大字节数（适配 link 帧 12B data 的 9B 剩余） */
#define EVENT_PAYLOAD_MAX  9

/* 跨核通配订阅 ID：订阅此"伪事件"可接收所有需跨核转发的事件 */
#define EVT_REMOTE_WILDCARD  0xFFFE

/* 订阅最大数量（按资源调整） */
#define EVENT_SUBSCRIBER_MAX  12

/* 订阅句柄（成功订阅返回 >=0 的句柄，用于退订；-1 表示失败） */
typedef int8_t event_sub_t;

/* ===================== API ===================== */

/* 初始化事件总线（init 表 SOFTWARE 阶段调用） */
void event_bus_init(void *arg);

/* 订阅指定事件。返回订阅句柄，<0 失败 */
event_sub_t event_bus_subscribe(uint16_t event_id, event_cb_t cb, void *user_data);

/* 订阅跨核通配（接收所有需跨核转发的事件，供 event_ipc 注册） */
event_sub_t event_bus_subscribe_remote(event_cb_t cb, void *user_data);

/* 退订 */
void event_bus_unsubscribe(event_sub_t sub);

/* 发布事件（本地分发 + 可选跨核转发，由 orient 控制）
 * @param orient   LOCAL: 仅本地分发; REMOTE: 仅投递对端; ANY: 两者皆做
 * @param payload  附加负载(<=EVENT_PAYLOAD_MAX 字节)，可为 NULL
 * @param len      负载长度
 * @param source   事件来源：发布者传 0(本地)；跨核桥接转发时传 1(远端)
 * @return 本地成功分发的订阅者数量；-1 失败
 */
int event_bus_publish(uint16_t event_id, event_orient_t orient,
                      const uint8_t *payload, uint8_t len, uint8_t source);

/* 便捷发布：本地 + 跨核全部生效（最常用） */
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

#endif /* EventBusUse */

#endif /* __EVENT_BUS_H_ */