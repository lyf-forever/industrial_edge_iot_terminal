#ifndef __CRED_MGR_H_
#define __CRED_MGR_H_

/**
 * @file cred_mgr.h
 * @brief 安全凭证管理器（架构 3.12）
 *
 * 集中管理 Wi-Fi 凭证、TLS 客户端证书、MQTT 密钥，存储于 NVS。
 * 提供按名读取/写入 API；凭证不导出（仅加载到内存使用）。
 *
 * 设计要点：
 *  - NVS 命名空间 "creds"，键 = 凭证名；
 *  - cred_load 拷贝到调用方缓冲（内存中短暂存在）；
 *  - 默认凭证在初始化时自动写入（首启）。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if CredUse

#define CRED_NAME_MAX     16
#define CRED_DATA_MAX     256

/* 内置凭证名 */
#define CRED_WIFI_SSID     "wifi_ssid"
#define CRED_WIFI_PSK      "wifi_psk"
#define CRED_MQTT_URI      "mqtt_uri"
#define CRED_MQTT_CLIENTID "mqtt_cid"

/* TLS 证书凭证名（mqtts:// 双向认证用） */
#define CRED_TLS_CA        "tls_ca"
#define CRED_TLS_CERT      "tls_cert"
#define CRED_TLS_KEY       "tls_key"

/* ===================== API ===================== */

/* 初始化（确保 NVS 可用；首启写入默认凭证），SOFTWARE 阶段 */
void cred_mgr_init(void *arg);

/* 读取凭证：name -> out[out_size]，返回字节数，-1 失败 */
int16_t cred_load(const char *name, uint8_t *out, uint16_t out_size);

/* 写入凭证：name <- data[n] */
bool cred_store(const char *name, const uint8_t *data, uint16_t n);

/* 读取字符串型凭证（自动补 '\0'） */
int16_t cred_load_str(const char *name, char *out, uint16_t out_size);

/* 写入字符串型凭证 */
bool cred_store_str(const char *name, const char *str);

/* 删除凭证 */
bool cred_erase(const char *name);

#endif /* CredUse */

#endif /* __CRED_MGR_H_ */