#ifndef __EVENT_ID_H_
#define __EVENT_ID_H_

/**
 * @file event_id.h
 * @brief 跨核共享事件 ID 枚举（GD32H7 端镜像，与 ESP32S3 端完全一致）
 *
 * 事件总线跨核化的"事件契约"：两端必须使用完全一致的事件 ID 编码，
 * 方可实现跨核透明发布订阅。新增事件 ID 时务必两端同步更新。
 */

#include <stdint.h>

typedef enum {
    EVT_NONE = 0x0000,

    /* ---- 传感器域 ---- */
    EVT_SENSOR_DATA       = 0x0100,   /* 传感器数据周期上报 (payload=sensor) */
    EVT_SENSOR_THRESHOLD  = 0x0101,   /* 越限事件 (payload=alarm) */
    EVT_SENSOR_ALARM      = 0x0102,   /* 报警 (payload=alarm) */
    EVT_SENSOR_ANOMALY    = 0x0103,   /* 异常检测结果 (payload=anomaly) */
    EVT_KWS               = 0x0104,   /* 唤醒词命中 */

    /* ---- 通信链路域 ---- */
    EVT_COMM_WIFI_STATE   = 0x0200,   /* Wi-Fi 状态变化 (payload=comm_state) */
    EVT_COMM_MQTT_STATE   = 0x0201,   /* MQTT 连接状态 (payload=comm_state) */
    EVT_COMM_LINK_STATE   = 0x0202,   /* 跨核链路状态 (payload=comm_state) */
    EVT_COMM_BLE_STATE    = 0x0203,   /* BLE 状态 (payload=comm_state) */

    /* ---- 控制域 ---- */
    EVT_CTRL_LED          = 0x0300,   /* LED 控制 (payload=led_ctrl) */
    EVT_CTRL_DISPLAY      = 0x0301,   /* 显示更新 (payload=display) */
    EVT_CTRL_ACTUATOR     = 0x0302,   /* 执行器动作 (payload=actuator) */

    /* ---- 系统域 ---- */
    EVT_SYS_HEARTBEAT     = 0x0400,   /* 心跳 (无 payload) */
    EVT_SYS_STATUS        = 0x0401,   /* 设备状态上报 (payload=status) */
    EVT_SYS_OTA_REQ       = 0x0402,   /* OTA 升级请求 */
    EVT_SYS_REBOOT        = 0x0403,   /* 重启 (本地/远程) */
    EVT_SYS_TIME_SYNC     = 0x0404,   /* 跨核时间同步 */
    EVT_SYS_OTA_RESULT    = 0x0405,   /* OTA 结果 (payload=ota_result) */
    EVT_SYS_LOG           = 0x0406,   /* 日志事件 (payload=日志文本) */
    EVT_SYS_ACL_DENIED    = 0x0407,   /* 权限拒绝告警 */

    EVT_MAX = 0xFFFF
} event_id_t;

/* 通信链路状态值 */
typedef enum {
    COMM_STATE_DOWN    = 0,
    COMM_STATE_CONNECTING,
    COMM_STATE_UP,
    COMM_STATE_ERROR
} comm_state_t;

/* 跨核事件路由标志 */
typedef enum {
    EVENT_ORIENT_LOCAL  = 0x01,
    EVENT_ORIENT_REMOTE = 0x02,
    EVENT_ORIENT_ANY    = 0x03
} event_orient_t;

#define EVENT_IS_REMOTE(id) ( ((id) & 0xFF00) == 0x0100 || \
                              ((id) & 0xFF00) == 0x0300 || \
                              ((id) & 0xFF00) == 0x0400 )

#endif /* __EVENT_ID_H_ */