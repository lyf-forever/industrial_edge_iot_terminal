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

static const char *TAG = "ota_dl";

#define OTA_RECV_BUF  4096

static int s_last_result = 0;
static uint32_t s_downloaded = 0;
static ota_progress_cb_t s_prog_cb = NULL;

/* HTTP 事件处理：流式写入 ota_mgr */
static esp_err_t http_evt_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (evt->data_len > 0) {
                int r = ota_mgr_feed((const uint8_t *)evt->data, evt->data_len);
                if (r != 0) {
                    ESP_LOGE(TAG, "ota_mgr_feed failed rc=%d", r);
                    s_last_result = OTA_RESULT_WRITE_ERR;
                    return ESP_FAIL;
                }
                s_downloaded += evt->data_len;
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
    s_last_result = 0;

    /* 启动 OTA 会话（获取备用分区 + esp_ota_begin） */
    if (ota_mgr_begin(url) != 0) {
        ESP_LOGE(TAG, "ota_mgr_begin failed");
        s_last_result = OTA_RESULT_DOWNLOAD_ERR;
        return -1;
    }

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
        if (ota_mgr_finish() != 0) {
            s_last_result = OTA_RESULT_VERIFY_ERR;
            return -2;
        }
        s_last_result = OTA_RESULT_OK;
        ESP_LOGI(TAG, "OTA download+verify OK");
        return 0;
    }

    s_last_result = OTA_RESULT_DOWNLOAD_ERR;
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