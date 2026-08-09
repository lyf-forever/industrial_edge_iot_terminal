#ifndef __SYS_H_
#define __SYS_H_

#include <stdio.h>
#include <stdint.h>
#include "esp_log.h"

/* 结构体 */


/* 传感器使用 */
#define SensorUse    1     /* 0: 不使用传感器 1：使用传感器 */
/* 传感器详情 */
#define Mq2Use       1     /* 0: 不使用MQ2 1：使用MQ2 */

/* 板级外设 */
#define Ws2812Use    1     /* 0: 不使用WS2812灯带 1：使用WS2812灯带 */

/* 串口链路（与 GD32H7 端通信，复用 link_protocol） */
#define LinkUse      1     /* 0: 不使用串口链路 1：使用串口链路 */

/* 无线通信 */
#define WiFiUse      1     /* 0: 不使用Wi-Fi 1：使用Wi-Fi */
#define MqttUse      1     /* 0: 不使用MQTT 1：使用MQTT (依赖 WiFiUse) */

/* 云端桥接（link<->MQTT 数据路由，依赖 LinkUse 与 MqttUse） */
#define CloudUse     1     /* 0: 不使用云端桥接 1：使用云端桥接 */

/* 事件总线（系统级消息中枢，跨核透明发布订阅） */
#define EventBusUse 1     /* 0: 不使用事件总线 1：使用事件总线 */

/* 连接层次状态机（HSM，订阅通信状态事件驱动指示灯） */
#define HsmUse      1     /* 0: 不使用HSM 1：使用HSM */

/* API declare */
void delay_us(uint32_t us);

#endif 