#ifndef __TOPIC_H_
#define __TOPIC_H_

/**
 * @file topic.h
 * @brief 主题化发布订阅（架构 3.2）
 *
 * 在事件总线之上提供"分层主题 + 前缀通配"的字符串视图：
 *   主题形如 "sensor.mq2" / "comm.wifi.state" / "ctrl.led"；
 *   订阅支持前缀通配 "sensor.*"。
 * 内部仍映射为 event_id 整数进行分发（二进制 ID 保持性能），
 * 主题是面向开发者的可读字符串视图。
 *
 * 兼容性：事件 ID 高 8 位=域、低 8 位=实例/动作。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"
#include "event_id.h"

#if TopicUse && EventBusUse

/* 主题最大长度 */
#define TOPIC_MAX_LEN  32
/* 通配符 */
#define TOPIC_WILDCARD  "*"

/* ===================== API ===================== */

/**
 * @brief 注册主题订阅（触发时以 source/event_id 分发到事件总线）
 * @param topic  "域.子域.实例" 或带 "*" 前缀通配
 * @param cb     回调（event_bus 风格）
 * @param user   用户上下文
 * @return 订阅句柄（event_sub_t），-1 失败
 */
int8_t topic_subscribe(const char *topic,
                       void (*cb)(uint16_t event_id, const uint8_t *payload,
                                  uint8_t len, uint8_t source, void *user),
                       void *user);

/* 退订 */
void topic_unsubscribe(int8_t sub);

/* 注册事件 ID <-> 主题双向映射（如 EVT_SENSOR_DATA -> "sensor.mq2"） */
bool topic_register_map(uint16_t event_id, const char *topic);

/* 将整数 event_id 映射为主题字符串（查映射表；未注册回退域级名） */
const char *topic_of_event(uint16_t event_id);

/* 主题 -> 事件 ID（精确匹配映射表；-1 未注册） */
int16_t topic_event_of(const char *topic);

/* 主题是否匹配（内部工具，含通配支持） */
bool topic_match(const char *pattern, const char *topic);

/* 由事件接收方调用：将事件向匹配的主题订阅者分发（供 event_ipc 等桥接使用） */
bool topic_dispatch(uint16_t event_id, const uint8_t *payload, uint8_t len,
                    uint8_t source, void *user);

#endif /* TopicUse && EventBusUse */

#endif /* __TOPIC_H_ */