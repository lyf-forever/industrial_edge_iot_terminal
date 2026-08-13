/**
 * @file ota_mgr.c
 * @brief A/B 鍒嗗尯 OTA 瀹炵幇锛歟sp_ota_* 娴佸紡鍐欏叆 + HSM 鐘舵€? */

#include "ota_mgr.h"

#if OtaUse

#include "esp_ota_ops.h"
#include "esp_log.h"
#include "hsm_fw.h"
#include "event_bus.h"
#include "event_id.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "ota";

/* ---- HSM 鐘舵€佽妭鐐?---- */
static hsm_node_t st_idle;
static hsm_node_t st_download;
static hsm_node_t st_verify;
static hsm_node_t st_apply;
static hsm_node_t st_rollback;

static hsm_t s_hsm;

/* esp_ota 鍙ユ焺 */
static esp_ota_handle_t s_ota_handle = 0;
static const esp_partition_t *s_update_partition = NULL;
static uint32_t s_received = 0;

static void publish_result(ota_result_t r)
{
    uint8_t p = (uint8_t)r;
    event_bus_publish_local(EVT_SYS_OTA_RESULT, &p, 1);
}

/* ---- HSM 鍔ㄤ綔 ---- */
static void st_idle_entry(hsm_t *me, void *payload, uint8_t len)
{
    (void)me; (void)payload; (void)len;
    ESP_LOGI(TAG, "state IDLE");
}

static void st_download_entry(hsm_t *me, void *payload, uint8_t len)
{
    (void)me; (void)payload; (void)len;
    ESP_LOGI(TAG, "state DOWNLOADING");
}

static void st_verify_entry(hsm_t *me, void *payload, uint8_t len)
{
    (void)me; (void)payload; (void)len;
    ESP_LOGI(TAG, "state VERIFYING");
}

static void st_apply_entry(hsm_t *me, void *payload, uint8_t len)
{
    (void)me; (void)payload; (void)len;
    ESP_LOGI(TAG, "state APPLYING");
}

static void st_rollback_entry(hsm_t *me, void *payload, uint8_t len)
{
    (void)me; (void)payload; (void)len;
    ESP_LOGI(TAG, "state ROLLBACK");
}

/* ---- 璺冭縼琛?---- */
static const hsm_trans_t s_idle_trans[] = {
    { EVT_SYS_OTA_REQ, &st_download },
};
static const hsm_trans_t s_download_trans[] = {
    { 0xF001, &st_verify },    /* 涓嬭浇瀹屾垚 */
    { 0xF002, &st_rollback },  /* 涓嬭浇澶辫触 */
};
static const hsm_trans_t s_verify_trans[] = {
    { 0xF003, &st_apply },     /* 鏍￠獙閫氳繃 */
    { 0xF004, &st_rollback },  /* 鏍￠獙澶辫触 */
};
static const hsm_trans_t s_apply_trans[] = {
    { 0xF005, &st_idle },      /* 鎻愪氦瀹屾垚锛堥噸鍚敱璋冪敤鏂癸級 */
};
static const hsm_trans_t s_rollback_trans[] = {
    { 0xF006, &st_idle },
};

void ota_mgr_init(void *arg)
{
    (void)arg;

    st_idle = (hsm_node_t){
        .name = "ota_idle", .parent = NULL,
        .on_entry = st_idle_entry, .trans = s_idle_trans, .trans_cnt = 1 };
    st_download = (hsm_node_t){
        .name = "ota_download", .parent = &st_idle,
        .on_entry = st_download_entry, .trans = s_download_trans, .trans_cnt = 2 };
    st_verify = (hsm_node_t){
        .name = "ota_verify", .parent = &st_idle,
        .on_entry = st_verify_entry, .trans = s_verify_trans, .trans_cnt = 2 };
    st_apply = (hsm_node_t){
        .name = "ota_apply", .parent = &st_idle,
        .on_entry = st_apply_entry, .trans = s_apply_trans, .trans_cnt = 1 };
    st_rollback = (hsm_node_t){
        .name = "ota_rollback", .parent = &st_idle,
        .on_entry = st_rollback_entry, .trans = s_rollback_trans, .trans_cnt = 1 };

    hsm_init(&s_hsm, &st_idle, NULL);
    ESP_LOGI(TAG, "init ok");
}

int ota_mgr_begin(const char *url)
{
    (void)url;
    s_update_partition = esp_ota_get_next_update_partition(NULL);
    if (s_update_partition == NULL) return -1;
    if (esp_ota_begin(s_update_partition, OTA_SIZE_UNKNOWN, &s_ota_handle) != ESP_OK) {
        return -2;
    }
    s_received = 0;
    hsm_dispatch(&s_hsm, EVT_SYS_OTA_REQ, NULL, 0);
    return 0;
}

int ota_mgr_feed(const uint8_t *data, uint32_t len)
{
    if (s_ota_handle == 0 || data == NULL) return -1;
    esp_err_t ret = esp_ota_write(s_ota_handle, data, len);
    if (ret != ESP_OK) {
        hsm_dispatch(&s_hsm, 0xF002, NULL, 0);
        publish_result(OTA_RESULT_WRITE_ERR);
        return -1;
    }
    s_received += len;
    return 0;
}

int ota_mgr_finish(void)
{
    if (s_ota_handle == 0) return -1;
    hsm_dispatch(&s_hsm, 0xF001, NULL, 0);   /* 涓嬭浇瀹屾垚 -> verify */

    esp_err_t ret = esp_ota_end(s_ota_handle);
    s_ota_handle = 0;
    if (ret != ESP_OK) {
        hsm_dispatch(&s_hsm, 0xF004, NULL, 0);
        publish_result(OTA_RESULT_VERIFY_ERR);
        return -1;
    }
    hsm_dispatch(&s_hsm, 0xF003, NULL, 0);   /* 鏍￠獙閫氳繃 -> apply */

    ret = esp_ota_set_boot_partition(s_update_partition);
    if (ret != ESP_OK) {
        hsm_dispatch(&s_hsm, 0xF004, NULL, 0);
        publish_result(OTA_RESULT_VERIFY_ERR);
        return -1;
    }
    hsm_dispatch(&s_hsm, 0xF005, NULL, 0);   /* apply 瀹屾垚 -> idle */
    publish_result(OTA_RESULT_OK);
    ESP_LOGI(TAG, "OTA finished, %lu bytes. reboot to apply.",
             (unsigned long)s_received);
    return 0;
}

void ota_mgr_abort(void)
{
    if (s_ota_handle != 0) {
        esp_ota_abort(s_ota_handle);
        s_ota_handle = 0;
    }
    publish_result(OTA_RESULT_ABORTED);
    hsm_transition(&s_hsm, &st_idle);
}

ota_state_t ota_mgr_state(void)
{
    const char *n = hsm_state_name(&s_hsm);
    if (n == st_download.name) return OTA_STATE_DOWNLOADING;
    if (n == st_verify.name)   return OTA_STATE_VERIFYING;
    if (n == st_apply.name)    return OTA_STATE_APPLYING;
    if (n == st_rollback.name) return OTA_STATE_ROLLBACK;
    return OTA_STATE_IDLE;
}

void ota_mgr_rollback(void)
{
    esp_ota_mark_app_invalid_rollback_and_reboot();
}

#endif /* OtaUse */