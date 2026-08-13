#ifndef __EVENT_IPC_H_
#define __EVENT_IPC_H_

/**
 * @file event_ipc.h
 * @brief 跨核事件桥接（GD32H7 端，架构 2.9 镜像）
 *
 * 将本地事件总线与串口链路（BT24 UART3 + link_protocol）桥接：
 *  - TX：订阅总线通配，需跨核事件编码为 LINK_CMD_EVENT 帧发送；
 *  - RX：link 帧回调，收到事件帧解码投递本地总线(source=1)。
 * 实现"跨核透明"发布订阅。
 */

#include <stdint.h>
#include <stdbool.h>

/* 初始化跨核桥接（订阅总线 + 注册 link 帧回调） */
void event_ipc_init(void);

/* 将一段接收字节流喂入 link 解析器（由 UART 接收处理调用） */
void event_ipc_feed(const uint8_t *data, uint16_t len);

/* 统计 */
uint32_t event_ipc_tx_count(void);
uint32_t event_ipc_rx_count(void);

#endif /* __EVENT_IPC_H_ */