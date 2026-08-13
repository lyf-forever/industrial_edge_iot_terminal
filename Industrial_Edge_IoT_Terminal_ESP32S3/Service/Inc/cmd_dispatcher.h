#ifndef __CMD_DISPATCHER_H_
#define __CMD_DISPATCHER_H_

/**
 * @file cmd_dispatcher.h
 * @brief 命令分发器（架构 3.7）
 *
 * 命令字 → 处理函数 + 权限标志的命令表分发机制。
 * 替代 cloud_bridge 内的 strstr 硬编码分支；新增命令仅追加表项。
 * 可选 ACL：need_cloud_auth 标志由调用方携带的 auth_level 校验。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if CmdUse

/* 命令处理函数：argv 指向参数区(未终止)，argc 参数个数 */
typedef void (*cmd_handler_t)(const uint8_t *argv, uint8_t argc);

/* 权限等级（与云端会话绑定） */
typedef enum {
    CMD_AUTH_NONE = 0,   /* 无鉴权（本地调试命令） */
    CMD_AUTH_CLOUD,      /* 云下发（默认） */
    CMD_AUTH_ADMIN       /* 管理员（OTA/凭证等敏感命令） */
} cmd_auth_level_t;

/* 命令表项 */
typedef struct {
    uint16_t      cmd_id;            /* 命令字 */
    cmd_auth_level_t need_auth;      /* 所需权限 */
    cmd_handler_t fn;
} cmd_entry_t;

/* 命令 ID 定义（新增命令在此扩展） */
enum {
    CMD_LED_CTRL     = 0x01,   /* LED 控制 */
    CMD_DISPLAY      = 0x02,   /* 显示更新 */
    CMD_REBOOT       = 0x03,   /* 远程重启 */
    CMD_SET_THRESHOLD= 0x04,   /* 阈值配置 */
    CMD_OTA_START    = 0x10,   /* OTA 启动 */
    CMD_OTA_ABORT    = 0x11,   /* OTA 中止 */
    CMD_GET_STATUS   = 0x20,   /* 状态查询 */
    CMD_CRED_UPDATE  = 0x30,   /* 凭证更新 */
};

/* ===================== API ===================== */

/**
 * @brief 分发命令
 * @param cmd_id 命令字
 * @param argv 参数指针（可 NULL）
 * @param argc 参数长度
 * @param auth 调用方权限等级；低于 need_auth 时拒绝并返回 -2
 * @return 0=已分发 1=未找到 -1=参数错 -2=权限不足
 */
int cmd_dispatch(uint16_t cmd_id, const uint8_t *argv, uint8_t argc,
                 cmd_auth_level_t auth);

/* 注册一个命令表（可多次调用合并多张表） */
void cmd_register_table(const cmd_entry_t *table, uint8_t count);

/* 查询命令所需权限（诊断用） */
cmd_auth_level_t cmd_need_auth(uint16_t cmd_id);

/* init 表接入：注册内建命令表 */
void cmd_disp_init(void *arg);

#endif /* CmdUse */

#endif /* __CMD_DISPATCHER_H_ */