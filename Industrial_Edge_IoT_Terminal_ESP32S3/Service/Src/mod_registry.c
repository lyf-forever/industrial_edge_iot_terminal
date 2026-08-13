/**
 * @file mod_registry.c
 * @brief 模块注册表与依赖拓扑排序实现
 *
 * 阶段内拓扑排序：反复扫描表，执行"依赖已满足"且"未执行"的项，
 * 每轮至少推进一项（或有项被跳过/失败），直到本轮无新进展。
 * 依赖通过 mod_id 登记的"已成功执行"位图判定。
 */

#include "mod_registry.h"

#if ModRegUse

#include <string.h>

/* 已成功完成的模块位图 */
static uint32_t s_done_map = 0;

/* 反射结果表（按表索引） */
static uint8_t s_results[64];   /* 支持最多 64 项 */

static const char *result_str(init_result_t r)
{
    switch (r) {
        case INIT_RESULT_OK:      return "OK";
        case INIT_RESULT_FAIL:    return "FAIL";
        case INIT_RESULT_SKIPPED: return "SKIP";
        default:                  return "PENDING";
    }
}

uint16_t mod_registry_run_stage(const mod_init_item_t *table, size_t count, uint8_t stage)
{
    if (table == NULL || count > 64) return 0;

    uint16_t done = 0;
    uint8_t  progress = 1;

    /* 每轮扫描，直到无新进展 */
    while (progress) {
        progress = 0;
        for (size_t i = 0; i < count; i++) {
            const mod_init_item_t *it = &table[i];
            if (it->stage != stage) continue;          /* 不属于本阶段 */
            if (s_results[i] != INIT_RESULT_PENDING) continue; /* 已处理 */

            /* 依赖检查：depends_on 中所有模块位必须已在 done_map，
             * 否则本轮跳过（依赖可能在后续轮次完成） */
            if ((it->depends_on & ~s_done_map) != 0) {
                continue;
            }

            ESP_LOGI("mod_reg", "[stage%d] %s...", stage, it->name);
            if (it->init_func) {
                it->init_func(it->arg);
            }
            s_results[i] = INIT_RESULT_OK;
            if (it->mod_id < MOD_MAX) {
                s_done_map |= BIT_MOD(it->mod_id);   /* 登记模块完成 */
            }
            done++;
            progress = 1;
        }
    }

    /* 本轮结束后仍 PENDING 的项：判定为依赖缺失(跳过) */
    for (size_t i = 0; i < count; i++) {
        if (s_results[i] == INIT_RESULT_PENDING && table[i].stage == stage) {
            ESP_LOGW("mod_reg", "[stage%d] %s skipped: missing dependency",
                     stage, table[i].name);
            s_results[i] = INIT_RESULT_SKIPPED;
        }
    }
    return done;
}

init_result_t mod_registry_result_of(const mod_init_item_t *table, size_t count,
                                     const char *name)
{
    if (table == NULL || name == NULL) return INIT_RESULT_MAX;
    for (size_t i = 0; i < count; i++) {
        if (table[i].name != NULL && strcmp(table[i].name, name) == 0) {
            return (init_result_t)s_results[i];
        }
    }
    return INIT_RESULT_MAX;
}

void mod_registry_dump(const mod_init_item_t *table, size_t count, uint8_t stage)
{
    if (table == NULL) return;
    for (size_t i = 0; i < count; i++) {
        if (table[i].stage != stage) continue;
        ESP_LOGI("mod_reg", "  %-16s -> %s", table[i].name,
                 result_str((init_result_t)s_results[i]));
    }
}

#endif /* ModRegUse */