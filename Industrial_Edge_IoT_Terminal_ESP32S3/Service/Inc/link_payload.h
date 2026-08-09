#ifndef __LINK_PAYLOAD_H_
#define __LINK_PAYLOAD_H_

/**
 * @file link_payload.h
 * @brief 串口链路载荷语义定义（ESP32S3 / GD32H7 两端共享）
 *
 * 本头文件定义 link_protocol 中 data[12] 字段的语义 overlay，
 * 以及设备地址与命令字枚举。两端 MCU 必须包含完全一致的定义，
 * 方可正确编解码串口链路帧的载荷。这是双 MCU 架构的核心复用点。
 *
 * 依赖：link_protocol.h（提供 LINK_MAX_DATA_LEN 等常量）。
 */

#include <stdint.h>
#include <stdbool.h>
#include "link_protocol.h"

/* ===================== 设备地址（两端共享） ========================= */
typedef enum {
    LINK_ADDR_ESP32S3 = 0x01,   /* 无线通信端 */
    LINK_ADDR_GD32H7  = 0x02,   /* 实时控制/显示端 */
    LINK_ADDR_BROADCAST = 0xFF  /* 广播地址 */
} link_addr_t;

/* ===================== 命令字（两端共享） =========================== */
typedef enum {
    LINK_CMD_HEARTBEAT   = 0x00,   /* 心跳 */
    LINK_CMD_SENSOR_DATA = 0x01,   /* 传感器数据上报 (GD32H7 -> ESP32S3) */
    LINK_CMD_ALARM       = 0x02,   /* 报警事件 (GD32H7 -> ESP32S3) */
    LINK_CMD_STATUS_REQ = 0x03,    /* 状态查询 (任一端 -> 对端) */
    LINK_CMD_STATUS_RSP = 0x04,    /* 状态回复 */
    LINK_CMD_LED_CTRL   = 0x10,    /* LED 控制 (ESP32S3 -> GD32H7，云端下发) */
    LINK_CMD_DISPLAY    = 0x11,    /* 显示更新 (ESP32S3 -> GD32H7，云端下发) */
    LINK_CMD_REBOOT     = 0x12,    /* 远程重启 */

    /* ---- 跨核事件总线载荷（事件传输专用） ---- */
    LINK_CMD_EVENT       = 0x20,    /* 跨核事件帧：data = event_payload_t */
    LINK_CMD_TIME_SYNC   = 0x21     /* 跨核时间同步 */
} link_cmd_t;

/* ===================== 传感器数据载荷 (12 字节 overlay) ============== *
 * 传感器数据帧 (cmd = LINK_CMD_SENSOR_DATA) 的 data 域语义。
 * 工业现场常用量：温度/湿度/气体浓度/CO2/气压。全部小端序。
 * 载荷总长固定为 LINK_MAX_DATA_LEN(12)，未用字段保留置 0。
 * */
#pragma pack(push, 1)
typedef struct __attribute__((packed)) {
    int16_t  temp_c;       /* 温度(℃)  x10，例如 256 = 25.6℃ */
    uint16_t humid_pct;    /* 湿度(%)  x10，例如 455 = 45.5% */
    uint16_t gas_ppm;      /* MQ2 可燃气体浓度(ppm) */
    uint16_t co2_ppm;      /* CO2 浓度(ppm) */
    uint16_t pressure_hpa; /* 气压(hPa) */
    uint16_t reserved;     /* 保留，置 0 */
} sensor_payload_t;
#pragma pack(pop)

/* 编译期断言：传感器载荷恰好填满 data[12]，避免两端结构体不一致 */
_Static_assert(sizeof(sensor_payload_t) == LINK_MAX_DATA_LEN,
                "sensor_payload_t must be exactly LINK_MAX_DATA_LEN bytes");

/* ===================== 报警载荷 (12 字节 overlay) ================== *
 * 报警帧 (cmd = LINK_CMD_ALARM) 的 data 域语义。
 * */
#pragma pack(push, 1)
typedef struct __attribute__((packed)) {
    uint8_t  alarm_id;    /* 报警编号 */
    uint8_t  alarm_level; /* 0:提示 1:警告 2:危险 */
    uint16_t sensor_val;  /* 触发报警的传感器原始值 */
    uint8_t  reserved[8]; /* 保留 */
} alarm_payload_t;
#pragma pack(pop)

_Static_assert(sizeof(alarm_payload_t) == LINK_MAX_DATA_LEN,
                "alarm_payload_t must be exactly LINK_MAX_DATA_LEN bytes");

/* ===================== LED 控制载荷 (12 字节 overlay) =============== *
 * LED 控制帧 (cmd = LINK_CMD_LED_CTRL) 的 data 域语义。
 * */
#pragma pack(push, 1)
typedef struct __attribute__((packed)) {
    uint8_t  led_id;     /* LED 编号 (见 GD32H7 端 led_target_t) */
    uint8_t  led_state;  /* 0:灭 1:亮 */
    uint8_t  reserved[10]; /* 保留 */
} led_ctrl_payload_t;
#pragma pack(pop)

_Static_assert(sizeof(led_ctrl_payload_t) == LINK_MAX_DATA_LEN,
                "led_ctrl_payload_t must be exactly LINK_MAX_DATA_LEN bytes");

/* ===================== 跨核事件载荷 (12 字节 overlay) =============== *
 * 跨核事件帧 (cmd = LINK_CMD_EVENT) 的 data 域语义。
 * 承载事件总线事件：event_id(2B) + 事件源标志(1B) + 负载(9B)。
 * 这就是"本地事件编码 → IPC 帧 → 对端解码投递到本地总线"的线缆格式。
 * */
#pragma pack(push, 1)
typedef struct __attribute__((packed)) {
    uint16_t event_id;       /* 事件 ID (见 event_id.h，两端共享) */
    uint8_t  source;         /* 事件源：0=本地产生 1=来自对端转发 (防环) */
    uint8_t  payload[9];     /* 事件附加负载(依赖事件类型，如 sensor/alarm overlay 头9字节) */
} event_payload_t;
#pragma pack(pop)

_Static_assert(sizeof(event_payload_t) == LINK_MAX_DATA_LEN,
                "event_payload_t must be exactly LINK_MAX_DATA_LEN bytes");

/* ===================== 通信状态载荷 (12 字节 overlay) =============== *
 * 通信状态事件 (EVT_COMM_*) 的 payload[9] 覆盖语义。
 * */
#pragma pack(push, 1)
typedef struct __attribute__((packed)) {
    uint8_t  link_id;        /* 链路编号: 0=wifi 1=mqtt 2=link 3=ble */
    uint8_t  state;          /* comm_state_t */
    uint16_t reserved;
    uint8_t  pad[6];
} comm_state_payload_t;
#pragma pack(pop)

/* ===================== 便捷工具函数声明 ============================ */

/**
 * @brief 将传感器载荷序列化进裸 data 数组
 * @param payload 传感器载荷结构体
 * @param data    目标缓冲区，须 >= LINK_MAX_DATA_LEN 字节
 */
void link_payload_sensor_pack(const sensor_payload_t *payload, uint8_t *data);

/**
 * @brief 从裸 data 数组反序列化传感器载荷
 * @param data    源数据，须 >= LINK_MAX_DATA_LEN 字节
 * @param payload 输出传感器载荷结构体
 */
void link_payload_sensor_unpack(const uint8_t *data, sensor_payload_t *payload);

/**
 * @brief 将报警载荷序列化进裸 data 数组
 */
void link_payload_alarm_pack(const alarm_payload_t *payload, uint8_t *data);

/**
 * @brief 从裸 data 数组反序列化报警载荷
 */
void link_payload_alarm_unpack(const uint8_t *data, alarm_payload_t *payload);

/**
 * @brief 将 LED 控制载荷序列化进裸 data 数组
 */
void link_payload_led_pack(const led_ctrl_payload_t *payload, uint8_t *data);

/**
 * @brief 从裸 data 数组反序列化 LED 控制载荷
 */
void link_payload_led_unpack(const uint8_t *data, led_ctrl_payload_t *payload);

/* 事件载荷编解码（跨核事件总线用） */
void link_payload_event_pack(const event_payload_t *payload, uint8_t *data);
void link_payload_event_unpack(const uint8_t *data, event_payload_t *payload);

#endif /* __LINK_PAYLOAD_H_ */