/**
 * @file cmd_dispatcher.c
 * @brief 命令分发器实现：静态命令表链
 */

#include "cmd_dispatcher.h"

#if CmdUse

#include "esp_log.h"
#include <stddef.h>

#define CMD_TABLES_MAX  4

typedef struct { const cmd_entry_t *table; uint8_t count; } cmd_table_ref_t;

static cmd_table_ref_t s_tables[CMD_TABLES_MAX];
static uint8_t s_table_cnt = 0;

void cmd_register_table(const cmd_entry_t *table, uint8_t count)
{
    if (table == NULL || s_table_cnt >= CMD_TABLES_MAX) return;
    s_tables[s_table_cnt].table = table;
    s_tables[s_table_cnt].count = count;
    s_table_cnt++;
}

static const cmd_entry_t *cmd_find(uint16_t cmd_id)
{
    for (uint8_t t = 0; t < s_table_cnt; t++) {
        for (uint8_t i = 0; i < s_tables[t].count; i++) {
            if (s_tables[t].table[i].cmd_id == cmd_id) {
                return &s_tables[t].table[i];
            }
        }
    }
    return NULL;
}

int cmd_dispatch(uint16_t cmd_id, const uint8_t *argv, uint8_t argc,
                 cmd_auth_level_t auth)
{
    const cmd_entry_t *e = cmd_find(cmd_id);
    if (e == NULL) return 1;
    if (auth < e->need_auth) {
        ESP_LOGW("cmd_disp", "cmd 0x%02X denied: auth=%d need=%d",
                 cmd_id, auth, e->need_auth);
        return -2;
    }
    if (e->fn) {
        e->fn(argv, argc);
        return 0;
    }
    return -1;
}

cmd_auth_level_t cmd_need_auth(uint16_t cmd_id)
{
    const cmd_entry_t *e = cmd_find(cmd_id);
    return e ? e->need_auth : CMD_AUTH_ADMIN;
}

/* init 表接入：注册业务命令表（由各业务模块提供的回调） */
static void cmd_disp_register_builtin(void);

void cmd_disp_init(void *arg)
{
    (void)arg;
    cmd_disp_register_builtin();
    ESP_LOGI("cmd_disp", "dispatcher ready");
}

/* 内建命令表：指向外部 handler（在 app_tasks 或 cloud_bridge 中定义） */
#include <stddef.h>
extern void cmd_handler_led(const uint8_t *argv, uint8_t argc);
extern void cmd_handler_reboot(const uint8_t *argv, uint8_t argc);
extern void cmd_handler_get_status(const uint8_t *argv, uint8_t argc);
extern void cmd_handler_cred_update(const uint8_t *argv, uint8_t argc);

static void cmd_disp_register_builtin(void)
{
    static const cmd_entry_t s_builtin[] = {
        { CMD_LED_CTRL,     CMD_AUTH_CLOUD, cmd_handler_led },
        { CMD_REBOOT,       CMD_AUTH_ADMIN, cmd_handler_reboot },
        { CMD_GET_STATUS,   CMD_AUTH_CLOUD, cmd_handler_get_status },
        { CMD_CRED_UPDATE,  CMD_AUTH_ADMIN, cmd_handler_cred_update },
    };
    cmd_register_table(s_builtin, sizeof(s_builtin) / sizeof(s_builtin[0]));
}

#endif /* CmdUse */