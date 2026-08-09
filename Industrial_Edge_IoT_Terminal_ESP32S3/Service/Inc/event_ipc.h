#ifndef __EVENT_IPC_H_
#define __EVENT_IPC_H_

/**
 * @file event_ipc.h
 * @brief 跨核事件桥接：本地事件总线 <-> link 串口帧
 *
 * 文档架构："跨核事件总线：本地事件编码 → IPC帧 → 对端解码投递到本地总线，
 * 实现跨核透明的发布订阅"。
 *
 * 实现机制：
 *  - TX：在本地事件总线上订阅通配(EVT_REMOTE_WILDCARD)，对于需跨核
 *    且来源==LOCAL 的事件，编码为 LINK_CMD_EVENT 帧经 bsp_uart 发往 GD32H7；
 *  - RX：作为 link_protocol 的帧回调，收到 LINK_CMD_EVENT 帧后解码为
 *    事件，以 source=REMOTE 投递到本地总线（不再回发，防环）。
 *
 * 这样业务模块只需 event_bus_publish，无需感知事件由哪颗芯片处理。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if EventBusUse && LinkUse

/* ===================== API ===================== */

/* 初始化跨核事件桥接（注册总线订阅 + link 帧回调），SOFTWARE 阶段调用 */
void event_ipc_init(void *arg);

/* （可选）任务：周期性时间同步/链路健康检测，由任务表创建 */
void event_ipc_task(void *arg);

/* 获取链路统计：收/发事件帧计数 */
uint32_t event_ipc_tx_count(void);
uint32_t event_ipc_rx_count(void);

#endif /* EventBusUse && LinkUse */

#endif /* __EVENT_IPC_H_ */