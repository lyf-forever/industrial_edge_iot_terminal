#ifndef __CONN_HSM_H_
#define __CONN_HSM_H_

/**
 * @file conn_hsm.h
 * @brief 连接层次状态机（HSM）
 *
 * 文档架构："层次状态机（HSM）通信状态管理，扩展到双模（蓝牙+Wi-Fi）
 * 复合状态与断线重连"。本模块以 HSM 管理系统连接状态：
 *
 * 层次状态：
 *   OFFLINE (顶层)
 *     ├── LINK_DOWN      跨核链路断
 *     └── ONLINE (跨核联通)
 *          ├── CLOUD_DISCONNECTED   (有链路但未上云)
 *          └── CLOUD_CONNECTED      (云端已连)
 *
 * HSM 订阅通信状态事件(EVT_COMM_*)，发生跃迁时发布新的状态事件，
 * 并驱动 WS2812 状态指示灯，实现"状态机驱动指示"的解耦设计。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if HsmUse

/* 系统连接顶层状态 */
typedef enum {
    HSM_STATE_OFFLINE = 0,        /* 跨核链路断 */
    HSM_STATE_CLOUD_DISCONNECTED, /* 跨核通但云端未连 */
    HSM_STATE_CLOUD_CONNECTED,    /* 全链路就绪 */
    HSM_STATE_MAX
} hsm_state_t;

/* ===================== API ===================== */

/* 初始化 HSM（订阅通信状态事件，SOFTWARE 阶段调用） */
void conn_hsm_init(void *arg);

/* 获取当前状态 */
hsm_state_t conn_hsm_get_state(void);

#endif /* HsmUse */

#endif /* __CONN_HSM_H_ */