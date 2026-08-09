/**
 * @file wifi_manager.c
 * @brief Wi-Fi 站点连接管理实现
 *
 * 基于 ESP-IDF 的 esp_wifi + netif + 事件循环，实现 STA 模式连接与
 * 断线自动重连。使用专用事件组向等待者同步连接状态。
 */

#include "wifi_manager.h"

#if WiFiUse

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "wifi_mgr";

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

static EventGroupHandle_t s_wifi_evt_group = NULL;
static wifi_state_t s_state = WIFI_STATE_DISCONNECTED;
static int s_retry_count = 0;
static wifi_state_cb_t s_state_cb = NULL;
static void *s_state_cb_user = NULL;
static esp_netif_t *s_sta_netif = NULL;

static void notify_state(wifi_state_t st)
{
    s_state = st;
    ESP_LOGI(TAG, "state -> %d", st);
    if (s_state_cb != NULL) s_state_cb(st, s_state_cb_user);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    (void)arg;
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                notify_state(WIFI_STATE_CONNECTING);
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                if (s_retry_count < WIFI_MANAGER_MAX_RETRY) {
                    s_retry_count++;
                    ESP_LOGW(TAG, "disconnected, retry %d/%d", s_retry_count, WIFI_MANAGER_MAX_RETRY);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    esp_wifi_connect();
                    notify_state(WIFI_STATE_CONNECTING);
                } else {
                    xEventGroupSetBits(s_wifi_evt_group, WIFI_FAIL_BIT);
                    notify_state(WIFI_STATE_FAILED);
                }
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&evt->ip_info.ip));
        s_retry_count = 0;
        xEventGroupSetBits(s_wifi_evt_group, WIFI_CONNECTED_BIT);
        notify_state(WIFI_STATE_CONNECTED);
    }
}

void wifi_manager_init(void *arg)
{
    (void)arg;

    /* NVS 初始化（Wi-Fi 配置存储依赖） */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    s_wifi_evt_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t inst_any_id;
    esp_event_handler_instance_t inst_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler, NULL, &inst_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler, NULL, &inst_got_ip));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = WIFI_MANAGER_SSID,
            .password = WIFI_MANAGER_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .scan_method = WIFI_FAST_SCAN,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_manager init done, connecting to SSID:%s", WIFI_MANAGER_SSID);
}

bool wifi_manager_wait_connected(uint32_t timeout_ms)
{
    if (s_wifi_evt_group == NULL) return false;

    EventBits_t bits = xEventGroupWaitBits(s_wifi_evt_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(timeout_ms));
    if (bits & WIFI_CONNECTED_BIT) {
        return true;
    }
    return false;
}

wifi_state_t wifi_manager_get_state(void)
{
    return s_state;
}

void wifi_manager_set_state_cb(wifi_state_cb_t cb, void *user_data)
{
    s_state_cb = cb;
    s_state_cb_user = user_data;
}

void wifi_manager_reconnect(void)
{
    s_retry_count = 0;
    if (s_wifi_evt_group != NULL) {
        xEventGroupClearBits(s_wifi_evt_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    }
    esp_wifi_connect();
    notify_state(WIFI_STATE_CONNECTING);
}

#endif /* WiFiUse */