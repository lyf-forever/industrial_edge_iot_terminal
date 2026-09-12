/**
 * @file ota_downloader.c
 * @brief OTA 固件下载循环实现（esp_http_client 流式下载）
 *
 * 事件驱动下载：HTTP 事件回调中按块接收数据，转发 ota_mgr_feed，
 * 下载结束调用 ota_mgr_finish 校验提交。支持基本认证与 TLS(https)。
 */

#include "ota_downloader.h"

#if OtaUse

#include "ota_mgr.h"
#include "event_bus.h"
#include "event_id.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#if MqttUse
#include "mqtt_client_app.h"
/* OTA 进度/结果上行（契约 6.1）：state 0待下载 1下载中 2校验切换 3成功 4失败 */
#define OTA_REPORT(state_, pct_, detail_)                                    \
    do {                                                                     \
        char j_[96];                                                         \
        int n_ = snprintf(j_, sizeof(j_),                                    \
            "{\"state\":%d,\"pct\":%d,\"detail\":\"%s\"}",                   \
            (state_), (pct_), (detail_));                                    \
        if (n_ > 0) mqtt_client_app_publish(MQTT_TOPIC_OTA_TX, j_, n_);      \
    } while (0)
#else
#define OTA_REPORT(state_, pct_, detail_) ((void)0)
#endif

static const char *TAG = "ota_dl";

#define OTA_RECV_BUF  4096
#define OTA_REPORT_STEP 65536   /* 进度上报节流：每 64KB */

static int s_last_result = 0;
static uint32_t s_downloaded = 0;
static uint32_t s_total_len = 0;      /* 由 Content-Length 头获取，未知为 0 */
static uint32_t s_last_report = 0;
static ota_progress_cb_t s_prog_cb = NULL;

static int ota_percent(void)
{
    if (s_total_len == 0) return -1;   /* 流式未知总长 */
    uint32_t pct = (uint32_t)((uint64_t)s_downloaded * 100u / s_total_len);
    return pct > 100 ? 100 : (int)pct;
}

/* HTTP 事件处理：流式写入 ota_mgr */
static esp_err_t http_evt_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ON_HEADER:
            if (evt->header_key && evt->header_value &&
                strcasecmp(evt->header_key, "Content-Length") == 0) {
                s_total_len = (uint32_t)strtoul(evt->header_value, NULL, 10);
            }
            break;
        case HTTP_EVENT_ON_DATA:
            if (evt->data_len > 0) {
                int r = ota_mgr_feed((const uint8_t *)evt->data, evt->data_len);
                if (r != 0) {
                    ESP_LOGE(TAG, "ota_mgr_feed failed rc=%d", r);
                    s_last_result = OTA_RESULT_WRITE_ERR;
                    return ESP_FAIL;
                }
                s_downloaded += evt->data_len;
                if (s_last_report == 0 || s_downloaded - s_last_report >= OTA_REPORT_STEP) {
                    s_last_report = s_downloaded;
                    OTA_REPORT(1, ota_percent(), "downloading");
                }
                if (s_prog_cb) {
                    s_prog_cb(s_downloaded, 0, NULL);   /* total 未知(HTTP 流式) */
                }
            }
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "download complete, %lu bytes", (unsigned long)s_downloaded);
            break;
        case HTTP_EVENT_ON_FINISH:
            break;
        default:
            break;
    }
    return ESP_OK;
}

int ota_downloader_start(const char *url, const char *username,
                         const char *password, ota_progress_cb_t cb)
{
    if (url == NULL) return -1;

    s_prog_cb = cb;
    s_downloaded = 0;
    s_total_len = 0;
    s_last_report = 0;
    s_last_result = 0;

    /* 启动 OTA 会话（获取备用分区 + esp_ota_begin） */
    if (ota_mgr_begin(url) != 0) {
        ESP_LOGE(TAG, "ota_mgr_begin failed");
        s_last_result = OTA_RESULT_DOWNLOAD_ERR;
        OTA_REPORT(4, -1, "begin failed");
        return -1;
    }
    OTA_REPORT(1, 0, "start");

    esp_http_client_config_t cfg = {
        .url = url,
        .event_handler = http_evt_handler,
        .timeout_ms = 10000,
        .buffer_size = OTA_RECV_BUF,
    };

    /* 可选基本认证 */
    if (username && password) {
        cfg.auth_type = HTTP_AUTH_TYPE_BASIC;
        cfg.username = username;
        cfg.password = password;
    }

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        s_last_result = OTA_RESULT_DOWNLOAD_ERR;
        return -1;
    }

    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    if (err == ESP_OK) {
        /* 下载完成：校验并提交 */
        OTA_REPORT(2, 100, "verifying");
        if (ota_mgr_finish() != 0) {
            s_last_result = OTA_RESULT_VERIFY_ERR;
            OTA_REPORT(4, 100, "verify failed");
            return -2;
        }
        s_last_result = OTA_RESULT_OK;
        OTA_REPORT(3, 100, "done");
        ESP_LOGI(TAG, "OTA download+verify OK");
        return 0;
    }

    s_last_result = OTA_RESULT_DOWNLOAD_ERR;
    OTA_REPORT(4, ota_percent(), "download error");
    ESP_LOGE(TAG, "HTTP perform failed: %s", esp_err_to_name(err));
    ota_mgr_abort();
    return (int)-err;
}

int ota_downloader_last_result(void) { return s_last_result; }

/* 独立 OTA 下载任务：云端命令触发后异步执行完整下载循环 */
void ota_download_task(void *arg)
{
    char *url = (char *)arg;
    ESP_LOGI(TAG, "OTA download task start: %s", url);
    int rc = ota_downloader_start(url, NULL, NULL, NULL);
    ESP_LOGI(TAG, "OTA download task done rc=%d", rc);
    vTaskDelete(NULL);
}

#endif /* OtaUse */