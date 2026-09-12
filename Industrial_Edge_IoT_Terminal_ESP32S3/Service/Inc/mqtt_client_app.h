#ifndef __MQTT_CLIENT_APP_H_
#define __MQTT_CLIENT_APP_H_

/**
 * @file mqtt_client_app.h
 * @brief MQTT 云端客户端服务
 *
 * 基于 ESP-IDF esp-mqtt 组件，负责与工业 IoT 云平台的连接、
 * 主题订阅与消息发布。上层（cloud_bridge）通过注册的接收回调处理下行命令。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if MqttUse

/* MQTT 代理配置（按现场云平台修改） */
#define MQTT_BROKER_URI       "mqtt://broker.emqx.io:1883"
#define MQTT_CLIENT_ID        "ind_edge_esp32s3_01"

/* 主题定义 */
#define MQTT_TOPIC_SENSORS_TX   "ind_edge/terminal/sensors"   /* 上行：传感器数据 */
#define MQTT_TOPIC_ALARM_TX     "ind_edge/terminal/alarm"     /* 上行：报警 */
#define MQTT_TOPIC_CMD_RX      "ind_edge/terminal/cmd"        /* 下行：控制命令(JSON) */
#define MQTT_TOPIC_STATUS_TX   "ind_edge/terminal/status"     /* 上行：状态/心跳 */
#define MQTT_TOPIC_OTA_TX      "ind_edge/terminal/ota"        /* 上行：OTA 进度/结果 */
#define MQTT_TOPIC_ACK_TX      "ind_edge/terminal/ack"        /* 上行：命令执行回执 */

/* QoS 级别 */
#define MQTT_QOS_TX   1
#define MQTT_QOS_RX   1

/* 接收回调：收到下行消息时调用上层 */
typedef void (*mqtt_msg_cb_t)(const char *topic, const char *data, int data_len, void *user_data);

/* ===================== API 声明 =============================== */

/* 初始化 MQTT 客户端（在初始化表 SOFTWARE 阶段，Wi-Fi 就绪后调用） */
void mqtt_client_app_init(void *arg);

/* 启动 MQTT 客户端（连接 broker）。需在 Wi-Fi 已连接后调用 */
void mqtt_client_app_start(void *arg);

/* 发布消息到指定主题 */
bool mqtt_client_app_publish(const char *topic, const char *data, int data_len);

/* 注册消息接收回调 */
void mqtt_client_app_set_msg_cb(mqtt_msg_cb_t cb, void *user_data);

/* 是否已连接 */
bool mqtt_client_app_is_connected(void);

#endif /* MqttUse */

#endif /* __MQTT_CLIENT_APP_H_ */