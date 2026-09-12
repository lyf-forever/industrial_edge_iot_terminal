#ifndef __NET_BRIDGE_H_
#define __NET_BRIDGE_H_

/**
 * @file net_bridge.h
 * @brief 无线透传桥：BLE GATT (Nordic UART Service) + LAN TCP Server + mDNS
 *
 * 契约（与 APP 对齐，见 docs/app_firmware_contract.docx）：
 *  - BLE 服务 6E400001-…，写特征 6E400002-…（APP→设备），
 *    通知特征 6E400003-…（设备→APP）；
 *  - TCP 端口 8080，mDNS 服务 _ind_edge._tcp；
 *  - 透传字节复用跨核 link_protocol 帧：无线侧收包 → event_ipc_feed_rx；
 *    跨核链路发帧 → net_bridge_broadcast（BLE notify + TCP 广播）。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if BleGattUse || TcpSrvUse

/* 初始化（SOFTWARE 阶段调用；内部按开关启动 BLE / TCP+mdns） */
void net_bridge_init(void *arg);

/* 无线侧收到的字节流喂入跨核链路解析（由 BLE 写事件 / TCP 收包调用） */
void net_bridge_feed_rx(const uint8_t *data, uint16_t len);

/* 跨核链路发出的帧镜像广播到无线侧（BLE notify + TCP 客户端） */
void net_bridge_broadcast(const uint8_t *data, uint16_t len);

#endif /* BleGattUse || TcpSrvUse */

#endif /* __NET_BRIDGE_H_ */
