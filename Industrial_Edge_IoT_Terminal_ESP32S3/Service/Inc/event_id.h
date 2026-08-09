#ifndef __EVENT_ID_H_
#define __EVENT_ID_H_

/**
 * @file event_id.h
 * @brief 跨核共享事件 ID 枚举（ESP32S3 / GD32H7 两端镜像）
 *
 * 文档架构核心："事件总线跨核化——模块订阅/发布事件时无需关心
 * 事件由哪颗芯片处理"。为达成跨核透明，两端必须使用完全一致的
 * 事件 ID 编码。本头文件是双 MCU 共享的"事件契约"。
 *
 * 命名规则：EVT_<域>_<动作/对象>
 *   - SENSOR   : 传感器数据/阈值
 *   - COMM     : 通信链路状态（Wi-Fi/MQTT/BLE/Link）
 *   - CONTROL  : 下行控制指令（LED/显示/执行器）
 *   - SYSTEM   : 系统级（心跳/OTA/重启/告警）
 *
 * 事件 ID 用 16-bit，跨核在 link 帧 data 域中承载（见 event_payload_t）。
 * 新增 ID 时务必在两端同步更新本文件并重新编译。
 */

#include <stdint.h>

/* ===================== 跨核共享事件 ID ===================== */
typedef enum {
    EVT_NONE = 0x0000,

    /* ---- 传感器域 (GD32H7 为主发布者) ---- */
    EVT_SENSOR_DATA       = 0x0100,   /* 传感器数据周期上报 (payload=sensor) */
    EVT_SENSOR_THRESHOLD  = 0x0101,   /* 越限事件 (payload=alarm) */
    EVT_SENSOR_ALARM      = 0x0102,   /* 报警 (payload=alarm) */

    /* ---- 通信链路域 ---- */
    EVT_COMM_WIFI_STATE   = 0x0200,   /* Wi-Fi 状态变化 (payload=comm_state) */
    EVT_COMM_MQTT_STATE  = 0x0201,   /* MQTT 连接状态 (payload=comm_state) */
    EVT_COMM_LINK_STATE  = 0x0202,   /* 跨核链路状态 (payload=comm_state) */
    EVT_COMM_BLE_STATE   = 0x0203,   /* BLE 状态 (payload=comm_state) */

    /* ---- 控制域 (云端/APP 为主发布者) ---- */
    EVT_CTRL_LED          = 0x0300,   /* LED 控制 (payload=led_ctrl) */
    EVT_CTRL_DISPLAY      = 0x0301,   /* 显示更新 (payload=display) */
    EVT_CTRL_ACTUATOR     = 0x0302,   /* 执行器动作 (payload=actuator) */

    /* ---- 系统域 ---- */
    EVT_SYS_HEARTBEAT     = 0x0400,   /* 心跳 (无 payload) */
    EVT_SYS_STATUS        = 0x0401,   /* 设备状态上报 (payload=status) */
    EVT_SYS_OTA_REQ       = 0x0402,   /* OTA 升级请求 */
    EVT_SYS_REBOOT        = 0x0403,   /* 重启 (本地/远程) */
    EVT_SYS_TIME_SYNC     = 0x0404,   /* 跨核时间同步 */

    EVT_MAX = 0xFFFF
} event_id_t;

/* ===================== 通信链路状态值 ===================== */
typedef enum {
    COMM_STATE_DOWN    = 0,    /* 断开 */
    COMM_STATE_CONNECTING,     /* 连接中 */
    COMM_STATE_UP,             /* 已连接 */
    COMM_STATE_ERROR           /* 错误/失败 */
} comm_state_t;

/* ===================== 跨核事件路由标志 ===================== *
 * 事件总线发布时可带 ORIENT 标志，指示事件传播方向：
 *   LOCAL  : 仅本地分发
 *   REMOTE : 同步转发到对端总线（跨核透明）
 *   ANY    : 本地分发 + 跨核转发
 * */
typedef enum {
    EVENT_ORIENT_LOCAL  = 0x01,   /* 仅本地总线 */
    EVENT_ORIENT_REMOTE = 0x02,   /* 仅投递到对端（不在本地分发） */
    EVENT_ORIENT_ANY    = 0x03    /* 本地分发 + 跨核转发 */
} event_orient_t;

/* 判断事件 ID 是否需要跨核（简单策略：传感器/控制/系统类跨核，通信状态通常本地） */
#define EVENT_IS_REMOTE(id) ( ((id) & 0xFF00) == 0x0100 || \
                              ((id) & 0xFF00) == 0x0300 || \
                              ((id) & 0xFF00) == 0x0400 )

#endif /* __EVENT_ID_H_ */