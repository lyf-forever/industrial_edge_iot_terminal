/**
 * @file mqtt_client_app.c
 * @brief MQTT 云端客户端实现
 *
 * 基于 ESP-IDF mqtt 组件。事件回调中处理连接成功/断开/数据接收，
 * 连接成功后自动订阅下行命令主题。
 */

#include "mqtt_client_app.h"

#if MqttUse

#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "mqtt_app";

static esp_mqtt_client_t s_client = NULL;
static bool s_connected = false;
static mqtt_msg_cb_t s_msg_cb = NULL;
static void *s_msg_cb_user = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    (void)base;
    esp_mqtt_event_handle_t evt = (esp_mqtt_event_handle_t)event_data;

    switch (evt->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected to broker");
            s_connected = true;
            esp_mqtt_client_subscribe(s_client, MQTT_TOPIC_CMD_RX, MQTT_QOS_RX);
            ESP_LOGI(TAG, "subscribed cmd topic: %s", MQTT_TOPIC_CMD_RX);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            s_connected = false;
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "subscribed, msg_id=%d", evt->msg_id);
            break;

        case MQTT_EVENT_DATA:
            if (s_msg_cb != NULL) {
                /* topic/data 未必以 \0 结尾，回调中需按长度处理 */
                s_msg_cb(evt->topic, evt->data, evt->data_len, s_msg_cb_user);
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error type=%d", evt->error_handle->error_type);
            break;

        default:
            break;
    }
}

void mqtt_client_app_init(void *arg)
{
    (void)arg;
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.client_id = MQTT_CLIENT_ID,
    };
    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_client == NULL) {
        ESP_LOGE(TAG, "esp_mqtt_client_init failed");
        return;
    }
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    ESP_LOGI(TAG, "MQTT client init done, broker=%s", MQTT_BROKER_URI);
}

void mqtt_client_app_start(void *arg)
{
    (void)arg;
    if (s_client == NULL) {
        ESP_LOGE(TAG, "client not init, cannot start");
        return;
    }
    esp_mqtt_client_start(s_client);
    ESP_LOGI(TAG, "MQTT client starting...");
}

bool mqtt_client_app_publish(const char *topic, const char *data, int data_len)
{
    if (s_client == NULL || !s_connected) return false;
    int msg_id = esp_mqtt_client_publish(s_client, topic, data, data_len, MQTT_QOS_TX, 0);
    return (msg_id >= 0);
}

void mqtt_client_app_set_msg_cb(mqtt_msg_cb_t cb, void *user_data)
{
    s_msg_cb = cb;
    s_msg_cb_user = user_data;
}

bool mqtt_client_app_is_connected(void)
{
    return s_connected;
}

#endif /* MqttUse */