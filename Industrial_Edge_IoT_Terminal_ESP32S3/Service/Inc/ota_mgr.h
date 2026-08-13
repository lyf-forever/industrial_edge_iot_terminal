#ifndef __OTA_MGR_H_
#define __OTA_MGR_H_

/**
 * @file ota_mgr.h
 * @brief A/B 分区 OTA 状态机（架构 3.11）
 *
 * 基于 esp_ota_* API：云端固件下载 → 校验 → 写入备用分区 →
 * 切换启动 → 重启验证 → 失败自动回滚。状态机用通用 HSM(3.10) 表达。
 *
 * 本版本为状态机框架 + esp_ota_* 落地骨架：
 *  - ota_mgr_begin(url)：发起下载（需 HTTP 客户端，此处留接口并直接
 *    以 chunk 流式写入；实际下载循环由调用方驱动 ota_mgr_feed）
 *  - ota_mgr_feed(data, len)：流式写入备用分区
 *  - ota_mgr_finish()：校验 + 提交 + 重启
 *  - ota_mgr_abort()：中止
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if OtaUse

/* OTA 状态（HSM 状态枚举） */
typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_DOWNLOADING,
    OTA_STATE_VERIFYING,
    OTA_STATE_APPLYING,      /* 提交并重启 */
    OTA_STATE_ROLLBACK,
    OTA_STATE_MAX
} ota_state_t;

/* OTA 结果事件（发布到事件总线 EVT_SYS_OTA_RESULT） */
typedef enum {
    OTA_RESULT_OK = 0,
    OTA_RESULT_DOWNLOAD_ERR,
    OTA_RESULT_VERIFY_ERR,
    OTA_RESULT_WRITE_ERR,
    OTA_RESULT_ABORTED
} ota_result_t;

/* ===================== API ===================== */

/* 初始化 OTA 管理器（SOFTWARE 阶段） */
void ota_mgr_init(void *arg);

/* 发起 OTA（url 为下载地址；返回 0=启动成功） */
int ota_mgr_begin(const char *url);

/* 流式喂入固件分片（下载循环内调用） */
int ota_mgr_feed(const uint8_t *data, uint32_t len);

/* 完成：校验并提交，必要时重启（返回 0=提交成功） */
int ota_mgr_finish(void);

/* 中止（释放资源，回退到 IDLE） */
void ota_mgr_abort(void);

/* 当前状态 */
ota_state_t ota_mgr_state(void);

/* 回滚到上一分区（启动失败时调用） */
void ota_mgr_rollback(void);

#endif /* OtaUse */

#endif /* __OTA_MGR_H_ */