#ifndef __CLOUD_BRIDGE_H_
#define __CLOUD_BRIDGE_H_

/**
 * @file cloud_bridge.h
 * @brief 云端桥接服务：link_protocol <-> MQTT 数据路由
 *
 * 作为应用层"大脑"，连接串口链路与云端：
 *  - 上行：link 收到 GD32H7 的传感器/报警帧 -> JSON 序列化 -> MQTT 发布
 *  - 下行：MQTT 收到控制命令(json) -> 解析 -> 通过 link 发往 GD32H7
 *
 * 复用 link_protocol / link_payload / bsp_uart / wifi_manager / mqtt_client_app。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if CloudUse

/* ===================== API 声明 =============================== */

/* 初始化桥接（注册各服务回调，在初始化表 SOFTWARE 阶段调用） */
void cloud_bridge_init(void *arg);

/* 桥接主任务：维持心跳、状态上报（由任务表创建） */
void cloud_bridge_task(void *arg);

#endif /* CloudUse */

#endif /* __CLOUD_BRIDGE_H_ */