#ifndef __OTA_DOWNLOADER_H_
#define __OTA_DOWNLOADER_H_

/**
 * @file ota_downloader.h
 * @brief OTA 固件下载循环（架构 3.11 完整落地）
 *
 * 基于 esp_http_client 实现云端固件下载 → 流式写入 ota_mgr_feed →
 * 完成校验。下载进度通过事件总线 EVT_SYS_OTA_RESULT 汇报。
 *
 * 依赖：ota_mgr（esp_ota 写入）、event_bus、cred_mgr（TLS 凭证可选）
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if OtaUse

/* 下载进度回调 */
typedef void (*ota_progress_cb_t)(uint32_t received, uint32_t total, void *user);

/* ===================== API ===================== */

/**
 * @brief 启动 OTA 下载任务（阻塞直到完成或失败）
 * @param url      固件下载 URL（http/https）
 * @param username 可选基本认证用户名（NULL=无）
 * @param password 可选基本认证密码
 * @param cb       进度回调（可 NULL）
 * @return 0=成功 负值=失败(负的 esp_err_t 取反)
 */
int ota_downloader_start(const char *url, const char *username,
                         const char *password, ota_progress_cb_t cb);

/* 查询上次下载结果 */
int ota_downloader_last_result(void);

#endif /* OtaUse */

#endif /* __OTA_DOWNLOADER_H_ */