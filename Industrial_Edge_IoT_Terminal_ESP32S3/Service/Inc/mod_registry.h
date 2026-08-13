#ifndef __MOD_REGISTRY_H_
#define __MOD_REGISTRY_H_

/**
 * @file mod_registry.h
 * @brief 模块/服务注册表与依赖声明（架构 3.1）
 *
 * 激活 initItem.depends_on 字段：引入模块编号枚举与依赖位掩码，
 * 由调度器在阶段内按拓扑序执行 init 表项；缺失依赖则跳过并告警。
 * 依赖关系（depends_on）用 BIT(mod_id) 位掩码表达。
 *
 * 设计要点：
 *  - 拓扑排序：阶段内反复扫描，仅执行"依赖已满足"的项；
 *  - 反射：记录每项执行结果(Drv_Err)，供日志导出与诊断；
 *  - 与现有 initItem/INIT_ITEM 兼容，仅增强调度器。
 */

#include <stdint.h>
#include <stddef.h>
#include "sys.h"
#include "esp_log.h"

#if ModRegUse

/* 模块编号（依赖位掩码的索引）。新增模块在此扩展 */
typedef enum {
    MOD_EVENT_BUS = 0,   /* 事件总线 */
    MOD_LINK,            /* 串口链路 */
    MOD_WIFI,            /* Wi-Fi */
    MOD_MQTT,            /* MQTT */
    MOD_CLOUD,           /* 云端桥接 */
    MOD_HSM,             /* 连接 HSM */
    MOD_MEMPOOL,         /* 内存池 */
    MOD_SOFT_TIMER,      /* 软定时器 */
    MOD_CHANNEL,         /* 通道抽象 */
    MOD_CMD,             /* 命令分发器 */
    MOD_LOG,             /* 结构化日志 */
    MOD_TTS,             /* 时间触发调度 */
    MOD_ACTOR,           /* Actor 框架 */
    MOD_CRED,            /* 凭证管理器 */
    MOD_OTA,             /* OTA */
    MOD_AI,              /* 边缘 AI */
    MOD_HSM_FW,          /* 通用 HSM 框架 */
    MOD_MAX              /* 总数 */
} mod_id_t;

/* 兼容别名 */
#define MOD_HSM_FW_DEP    MOD_HSM_FW

/* 依赖位掩码助手 */
#define BIT_MOD(id)   (1UL << (id))
#define MOD_DEP(...)  /* 由调用处手工组合 BIT_MOD() */

/* 初始化项执行结果（反射表） */
typedef enum {
    INIT_RESULT_PENDING = 0,   /* 未执行 */
    INIT_RESULT_OK,            /* 执行成功 */
    INIT_RESULT_FAIL,          /* 执行失败 */
    INIT_RESULT_SKIPPED,       /* 依赖缺失被跳过 */
    INIT_RESULT_MAX
} init_result_t;

/* 模块初始化表项（与 main.c initItem 兼容的增强版） */
typedef struct {
    const char *name;          /* 步骤名称 */
    uint8_t     stage;         /* 阶段（沿用 initStage 值） */
    void      (*init_func)(void *arg);
    void       *arg;
    uint32_t    depends_on;    /* 依赖位掩码：BIT_MOD(MOD_xxx) 组合，0=无依赖 */
    mod_id_t    mod_id;        /* 本模块编号（用于被依赖判定），MOD_MAX 表示不注册 */
} mod_init_item_t;

/* 便捷宏 */
#define MOD_INIT_ITEM(_name, _stage, _func, _arg, _dep, _mid) \
    { .name = (_name), .stage = (_stage), .init_func = (_func), \
      .arg = (_arg), .depends_on = (_dep), .mod_id = (_mid) }

/* ===================== API ===================== */

/**
 * @brief 按拓扑序执行初始化表
 * @param table 初始化表
 * @param count 表项数量
 * @param stage 本阶段枚举（HARDWARE=0 / SOFTWARE=1）
 * @return 本阶段成功执行项数
 */
uint16_t mod_registry_run_stage(const mod_init_item_t *table, size_t count, uint8_t stage);

/* 查询某项执行结果 */
init_result_t mod_registry_result_of(const mod_init_item_t *table, size_t count,
                                     const char *name);

/* 打印阶段内所有项的反射结果（诊断用） */
void mod_registry_dump(const mod_init_item_t *table, size_t count, uint8_t stage);

#endif /* ModRegUse */

#endif /* __MOD_REGISTRY_H_ */