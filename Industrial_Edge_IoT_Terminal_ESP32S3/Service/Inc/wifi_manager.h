#ifndef __WIFI_MANAGER_H_
#define __WIFI_MANAGER_H_

/**
 * @file wifi_manager.h
 * @brief Wi-Fi 站点连接管理服务
 *
 * 封装 ESP-IDF Wi-Fi STA 模式的连接、断线重连逻辑与事件回调。
 * 上层（cloud_bridge）通过 wifi_manager_wait_connected 阻塞等待联网，
 * 或通过注册回调获知连接状态变化。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if WiFiUse

/* Wi-Fi 配置宏（按现场网络修改） */
#define WIFI_MANAGER_SSID       "IndustrialEdge"
#define WIFI_MANAGER_PASSWORD   "12345678"
#define WIFI_MANAGER_MAX_RETRY  5        /* 最大重连次数 */
#define WIFI_MANAGER_CONN_TIMEOUT_MS 15000 /* 连接等待超时 */

/* 连接状态 */
typedef enum {
    WIFI_STATE_DISCONNECTED = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_FAILED
} wifi_state_t;

/* 状态变化回调（上层可注册以联动 MQTT 等服务） */
typedef void (*wifi_state_cb_t)(wifi_state_t state, void *user_data);

/* ===================== API 声明 =============================== */

/* 初始化 Wi-Fi 站点模式（在初始化表 HARDWARE 阶段调用） */
void wifi_manager_init(void *arg);

/* 阻塞等待连接成功，超时返回 false */
bool wifi_manager_wait_connected(uint32_t timeout_ms);

/* 获取当前状态 */
wifi_state_t wifi_manager_get_state(void);

/* 注册状态变化回调 */
void wifi_manager_set_state_cb(wifi_state_cb_t cb, void *user_data);

/* 触发重连 */
void wifi_manager_reconnect(void);

#endif /* WiFiUse */

#endif /* __WIFI_MANAGER_H_ */