/**
 * @file cred_mgr.c
 * @brief 凭证管理器实现：NVS 命名空间封装
 */

#include "cred_mgr.h"

#if CredUse

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "cred";
static nvs_handle_t s_nvs = 0;
static bool s_ready = false;

/* 默认凭证（首启写入） */
static const struct { const char *name; const char *value; } s_defaults[] = {
    { CRED_WIFI_SSID,      "IndustrialEdge" },
    { CRED_WIFI_PSK,       "12345678" },
    { CRED_MQTT_URI,       "mqtt://broker.emqx.io:1883" },
    { CRED_MQTT_CLIENTID,  "ind_edge_esp32s3_01" },
};

void cred_mgr_init(void *arg)
{
    (void)arg;
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    ret = nvs_open("creds", NVS_READWRITE, &s_nvs);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open creds failed: %d", ret);
        return;
    }
    s_ready = true;

    /* 首启写入默认凭证 */
    for (size_t i = 0; i < sizeof(s_defaults) / sizeof(s_defaults[0]); i++) {
        size_t len = 0;
        if (nvs_get_str(s_nvs, s_defaults[i].name, NULL, &len) != ESP_OK) {
            nvs_set_str(s_nvs, s_defaults[i].name, s_defaults[i].value);
        }
    }
    nvs_commit(s_nvs);
    ESP_LOGI(TAG, "init ok (%d defaults)", 
             (int)(sizeof(s_defaults) / sizeof(s_defaults[0])));
}

int16_t cred_load(const char *name, uint8_t *out, uint16_t out_size)
{
    if (!s_ready || name == NULL || out == NULL || out_size == 0) return -1;
    size_t len = out_size;
    esp_err_t ret = nvs_get_blob(s_nvs, name, out, &len);
    if (ret != ESP_OK) {
        /* 回退：若以字符串存储的读为字符串 */
        char str[CRED_DATA_MAX];
        size_t slen = sizeof(str);
        if (nvs_get_str(s_nvs, name, str, &slen) == ESP_OK) {
            if (slen > out_size) slen = out_size;
            memcpy(out, str, slen);
            return (int16_t)slen;
        }
        return -1;
    }
    return (int16_t)len;
}

bool cred_store(const char *name, const uint8_t *data, uint16_t n)
{
    if (!s_ready || name == NULL || data == NULL || n > CRED_DATA_MAX) return false;
    if (nvs_set_blob(s_nvs, name, data, n) != ESP_OK) return false;
    return nvs_commit(s_nvs) == ESP_OK;
}

int16_t cred_load_str(const char *name, char *out, uint16_t out_size)
{
    if (!s_ready || name == NULL || out == NULL || out_size == 0) return -1;
    size_t len = out_size;
    if (nvs_get_str(s_nvs, name, out, &len) != ESP_OK) return -1;
    return (int16_t)(len > 0 ? len - 1 : 0);   /* 不含 '\0' */
}

bool cred_store_str(const char *name, const char *str)
{
    if (!s_ready || name == NULL || str == NULL) return false;
    if (nvs_set_str(s_nvs, name, str) != ESP_OK) return false;
    return nvs_commit(s_nvs) == ESP_OK;
}

bool cred_erase(const char *name)
{
    if (!s_ready || name == NULL) return false;
    if (nvs_erase_key(s_nvs, name) != ESP_OK) return false;
    return nvs_commit(s_nvs) == ESP_OK;
}

#endif /* CredUse */