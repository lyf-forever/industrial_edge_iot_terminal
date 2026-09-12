/**
 * @file cloud_bridge.c
 * @brief 云端桥接服务实现（事件总线版）
 *
 * 基于 event_bus 解耦：cloud_bridge 现在是事件总线的订阅者 + 发布者，
 * 不再硬编码 link 帧回调到 mqtt 的分支逻辑，符合文档"模块订阅/发布
 * 事件无需感知事件由哪颗芯片处理"的跨核透明理念。
 *
 * 角色重构：
 *   - 订阅 EVT_SENSOR_DATA/EVT_SENSOR_ALARM -> JSON 上报 MQTT (上行)
 *   - 订阅 EVT_SYS_HEARTBEAT -> 状态上报 MQTT (上行)
 *   - MQTT 下行命令 -> 发布 EVT_CTRL_LED/EVT_SYS_REBOOT 到总线(->跨核)
 *   - Wi-Fi 状态回调 -> 发布 EVT_COMM_WIFI_STATE 到本地总线
 *   - MQTT 连接状态 -> 发布 EVT_COMM_MQTT_STATE 到本地总线
 *
 * 事件总线的通配订阅者(event_ipc)负责把需跨核的事件编码成 link 帧发往
 * 对端；cloud_bridge 不再关心跨核细节。
 */

#include "cloud_bridge.h"
#include "sys.h"

#if CloudUse

#if WiFiUse
#include "wifi_manager.h"
#endif
#if MqttUse
#include "mqtt_client_app.h"
#endif
#if EventBusUse
#include "event_bus.h"
#include "event_id.h"
#include "link_payload.h"
#endif
#if LinkUse
#include "link_protocol.h"
#include "bsp_uart.h"
#endif
#if CmdUse
#include "cmd_dispatcher.h"
#endif
#if CredUse
#include "cred_mgr.h"
#endif
#if OtaUse
#include "ota_mgr.h"
#endif
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "cloud_br";

/* ===================== 事件总线订阅者句柄 ===================== */
#if EventBusUse
static event_sub_t sub_sensor = -1;
static event_sub_t sub_alarm  = -1;
static event_sub_t sub_hb     = -1;
#endif

/* ===================== 上行：传感器事件 -> MQTT JSON ================== */
#if EventBusUse
static void on_sensor_data(uint16_t event_id, const uint8_t *payload,
                           uint8_t len, uint8_t source, void *user_data)
{
    (void)event_id; (void)len; (void)source; (void)user_data;
#if MqttUse
    if (payload == NULL || len < (int)sizeof(sensor_payload_t)) return;
    const sensor_payload_t *sp = (const sensor_payload_t *)payload;
    char json[160];
    int n = snprintf(json, sizeof(json),
        "{\"temp\":%.1f,\"humid\":%.1f,\"gas\":%u,\"co2\":%u,\"press\":%u}",
        sp->temp_c / 10.0f, sp->humid_pct / 10.0f,
        sp->gas_ppm, sp->co2_ppm, sp->pressure_hpa);
    if (n > 0) {
        mqtt_client_app_publish(MQTT_TOPIC_SENSORS_TX, json, n);
        ESP_LOGI(TAG, "TX sensors -> %s", json);
    }
#endif
}

static void on_sensor_alarm(uint16_t event_id, const uint8_t *payload,
                            uint8_t len, uint8_t source, void *user_data)
{
    (void)event_id; (void)len; (void)source; (void)user_data;
#if MqttUse
    if (payload == NULL || len < (int)sizeof(alarm_payload_t)) return;
    const alarm_payload_t *ap = (const alarm_payload_t *)payload;
    char json[160];
    int n = snprintf(json, sizeof(json),
        "{\"alarm_id\":%u,\"level\":%u,\"val\":%u}",
        ap->alarm_id, ap->alarm_level, ap->sensor_val);
    if (n > 0) {
        mqtt_client_app_publish(MQTT_TOPIC_ALARM_TX, json, n);
        ESP_LOGW(TAG, "TX alarm -> %s", json);
    }
#endif
}

static void on_heartbeat(uint16_t event_id, const uint8_t *payload,
                        uint8_t len, uint8_t source, void *user_data)
{
    (void)event_id; (void)payload; (void)len; (void)source; (void)user_data;
#if MqttUse
    if (!mqtt_client_app_is_connected()) return;
    char json[128];
    int n = snprintf(json, sizeof(json),
        "{\"heartbeat\":%u,\"uptime_s\":%u,\"ver\":\"%s\"}",
        (unsigned)(xTaskGetTickCount() / configTICK_RATE_HZ / 10),   /* 粗略计数 */
        (unsigned)(xTaskGetTickCount() / configTICK_RATE_HZ),
        AppFwVersion);
    if (n > 0) {
        mqtt_client_app_publish(MQTT_TOPIC_STATUS_TX, json, n);
    }
#endif
}
#endif /* EventBusUse */

/* ===================== 命令处理器（供命令分发器调用） ============== *
 * 由 cmd_dispatcher 的命令表注册；通过事件总线发布控制事件。
 * */
#if CmdUse
void cmd_handler_led(const uint8_t *argv, uint8_t argc)
{
    (void)argc;
    if (argv == NULL) return;
    /* argv 为 JSON 片段 {"cmd":"led","id":1,"state":1}，简单解析 */
    led_ctrl_payload_t led = {0};
    char buf[64];
    int cp = argc < (int)sizeof(buf) - 1 ? argc : (int)sizeof(buf) - 1;
    memcpy(buf, argv, cp);
    buf[cp] = '\0';
    char *p_id    = strstr(buf, "\"id\":");
    char *p_state = strstr(buf, "\"state\":");
    if (p_id)    led.led_id    = (uint8_t)atoi(p_id + 5);
    if (p_state) led.led_state = (uint8_t)atoi(p_state + 8);
    event_bus_publish_any(EVT_CTRL_LED, (const uint8_t *)&led, sizeof(led));
    ESP_LOGI(TAG, "CMD led (id=%d state=%d)", led.led_id, led.led_state);
}

void cmd_handler_reboot(const uint8_t *argv, uint8_t argc)
{
    (void)argv; (void)argc;
    event_bus_publish_any(EVT_SYS_REBOOT, NULL, 0);
    ESP_LOGI(TAG, "CMD reboot");
}

void cmd_handler_get_status(const uint8_t *argv, uint8_t argc)
{
    (void)argv; (void)argc;
    uint8_t st = 1;
    event_bus_publish_local(EVT_SYS_STATUS, &st, 1);
    ESP_LOGI(TAG, "CMD get_status");
}

#if CredUse
/* 凭证更新命令：{"cred":"tls_ca","data":"<pem 内容>"}
 * 解析后写入 NVS（架构 3.12：云端下发证书/凭证轮换）。 */
void cmd_handler_cred_update(const uint8_t *argv, uint8_t argc)
{
    if (argv == NULL || argc == 0) return;
    char buf[1024];
    int cp = argc < (int)sizeof(buf) - 1 ? argc : (int)sizeof(buf) - 1;
    memcpy(buf, argv, cp);
    buf[cp] = '\0';

    char *p_cred = strstr(buf, "\"cred\":\"");
    char *p_data = strstr(buf, "\"data\":\"");
    if (p_cred == NULL || p_data == NULL) {
        ESP_LOGW(TAG, "CMD cred_update: bad format");
        return;
    }
    char name[32];
    char *name_end = strchr(p_cred + 8, '"');
    if (name_end == NULL || (name_end - (p_cred + 8)) >= (int)sizeof(name)) {
        return;
    }
    memcpy(name, p_cred + 8, (size_t)(name_end - (p_cred + 8)));
    name[name_end - (p_cred + 8)] = '\0';

    char *data = p_data + 8;
    char *data_end = strchr(data, '"');
    if (data_end == NULL) return;
    int data_len = (int)(data_end - data);
    if (data_len <= 0 || data_len > CRED_DATA_MAX) {
        ESP_LOGW(TAG, "CMD cred_update: bad data len=%d", data_len);
        return;
    }
    if (cred_store(name, (const uint8_t *)data, (uint16_t)data_len)) {
        ESP_LOGI(TAG, "CMD cred_update: %s stored (%d bytes)", name, data_len);
    } else {
        ESP_LOGE(TAG, "CMD cred_update: store failed");
    }
}
#else
void cmd_handler_cred_update(const uint8_t *argv, uint8_t argc)
{
    (void)argv; (void)argc;
    ESP_LOGW(TAG, "CMD cred_update: CredUse disabled");
}
#endif /* CredUse */
#endif /* CmdUse */

/* ===================== 下行：MQTT 消息回调 -> 发布事件 ============= */
#if MqttUse
static void on_mqtt_msg(const char *topic, const char *data, int data_len, void *user_data)
{
    (void)user_data;
    ESP_LOGI(TAG, "RX mqtt topic=%s len=%d", topic, data_len);

    char buf[128];
    int cp = data_len < (int)sizeof(buf) - 1 ? data_len : (int)sizeof(buf) - 1;
    memcpy(buf, data, cp);
    buf[cp] = '\0';

#if CmdUse
    /* 架构 3.7：命令分发器查表路由（取代 strstr 硬编码分支） */
    {
        uint16_t cmd_id = 0;
        const char *cmd_name = "";
        if      (strstr(buf, "\"led\"")    != NULL) { cmd_id = CMD_LED_CTRL;   cmd_name = "led"; }
        else if (strstr(buf, "\"reboot\"") != NULL) { cmd_id = CMD_REBOOT;     cmd_name = "reboot"; }
        else if (strstr(buf, "\"status\"") != NULL) { cmd_id = CMD_GET_STATUS; cmd_name = "status"; }
        else if (strstr(buf, "\"ota\"")    != NULL) { cmd_id = CMD_OTA_START;  cmd_name = "ota"; }

        if (cmd_id != 0) {
            int r = cmd_dispatch(cmd_id, (const uint8_t *)buf, (uint8_t)cp,
                                 CMD_AUTH_CLOUD);
            if (r == 1) {
                ESP_LOGW(TAG, "cmd 0x%02X not registered", cmd_id);
            }
            /* 命令执行回执上行（与 QoS 送达确认区分，见契约 6.2） */
            char ack[64];
            int an = snprintf(ack, sizeof(ack), "{\"cmd\":\"%s\",\"result\":%d}",
                              cmd_name, r);
            if (an > 0) {
                mqtt_client_app_publish(MQTT_TOPIC_ACK_TX, ack, an);
            }
#if OtaUse
            if (cmd_id == CMD_OTA_START && r == 0) {
                /* OTA 启动：从 JSON 提取 URL，独立任务执行完整下载循环
                 * （避免阻塞 MQTT 回调上下文） */
                char *p_url = strstr(buf, "\"url\":\"");
                if (p_url) {
                    char url[96];
                    char *end = strchr(p_url + 7, '"');
                    if (end && (end - (p_url + 7)) < (int)sizeof(url)) {
                        memcpy(url, p_url + 7, (size_t)(end - (p_url + 7)));
                        url[end - (p_url + 7)] = '\0';
                        static char ota_url[96];
                        snprintf(ota_url, sizeof(ota_url), "%s", url);
                        extern void ota_download_task(void *arg);
                        xTaskCreate(ota_download_task, "otaTask", 8192,
                                    ota_url, 8, NULL);
                    }
                }
            }
#endif
            return;
        }
    }
#endif /* CmdUse */

#if EventBusUse
    /* 其余命令（如 display）直接发布事件 */
    if (strstr(buf, "\"display\"") != NULL) {
        event_bus_publish_any(EVT_CTRL_DISPLAY, (const uint8_t *)buf, cp);
    }
#else
    /* 无事件总线时的直发回退路径(link) */
#if LinkUse
    if (strstr(buf, "\"led\"") != NULL) {
        led_ctrl_payload_t led = {0};
        char *p_id = strstr(buf, "\"id\":");
        char *p_state = strstr(buf, "\"state\":");
        if (p_id)    led.led_id = (uint8_t)atoi(p_id + 5);
        if (p_state) led.led_state = (uint8_t)atoi(p_state + 8);
        uint8_t pl[LINK_MAX_DATA_LEN], frame[40];
        link_payload_led_pack(&led, pl);
        uint16_t flen = build_frame(LINK_ADDR_ESP32S3, LINK_CMD_LED_CTRL,
                                    pl, LINK_MAX_DATA_LEN, frame, sizeof(frame));
        if (flen > 0) bsp_uart_send(frame, flen);
    }
#endif
#endif /* EventBusUse */
}
#endif /* MqttUse */

/* ===================== Wi-Fi 状态回调 -> 发布通信事件 ============== */
#if WiFiUse
static void on_wifi_state(wifi_state_t state, void *user_data)
{
    (void)user_data;
    uint8_t wifi_st = (uint8_t)state;   /* wifi_state_t 与 comm_state_t 数值兼容 */
#if EventBusUse
    comm_state_payload_t cs = { .link_id = 0, .state = wifi_st, .reserved = 0 };
    event_bus_publish_local(EVT_COMM_WIFI_STATE, (uint8_t *)&cs, sizeof(cs));
#endif
    if (state == WIFI_STATE_CONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi connected, starting MQTT");
#if MqttUse
        mqtt_client_app_start(NULL);
#endif
    }
}
#endif

static void mqtt_publish_conn_state(bool up)
{
#if EventBusUse
    comm_state_payload_t cs = { .link_id = 1, .state = up ? COMM_STATE_UP : COMM_STATE_DOWN };
    event_bus_publish_local(EVT_COMM_MQTT_STATE, (uint8_t *)&cs, sizeof(cs));
#endif
}

void cloud_bridge_init(void *arg)
{
    (void)arg;

#if EventBusUse
    sub_sensor = event_bus_subscribe(EVT_SENSOR_DATA, on_sensor_data, NULL);
    sub_alarm  = event_bus_subscribe(EVT_SENSOR_ALARM, on_sensor_alarm, NULL);
    sub_hb     = event_bus_subscribe(EVT_SYS_HEARTBEAT, on_heartbeat, NULL);
#endif
    /* link 帧回调由 event_ipc 注册(事件总线版)或回退直发(无总线版) */

#if WiFiUse
    wifi_manager_set_state_cb(on_wifi_state, NULL);
#endif

#if MqttUse
    mqtt_client_app_set_msg_cb(on_mqtt_msg, NULL);
    /* 用 MQTT 连接状态回调发布 EVT_COMM_MQTT_STATE（简单实现：轮询） */
#endif

    ESP_LOGI(TAG, "cloud_bridge init done (eventbus=%d)", EventBusUse);
}

void cloud_bridge_task(void *arg)
{
    (void)arg;

#if WiFiUse
    if (!wifi_manager_wait_connected(WIFI_MANAGER_CONN_TIMEOUT_MS)) {
        ESP_LOGE(TAG, "Wi-Fi connect timeout, bridge task in degraded mode");
    }
#endif

    const TickType_t period = pdMS_TO_TICKS(10000);

    while (1) {
        vTaskDelay(period);
#if MqttUse
        /* 周期性同步 MQTT 连接状态到事件总线（供 conn_hsm） */
        static bool last_up = false;
        bool now_up = mqtt_client_app_is_connected();
        if (now_up != last_up) {
            mqtt_publish_conn_state(now_up);
            last_up = now_up;
        }
#endif
    }
}

#endif /* CloudUse */