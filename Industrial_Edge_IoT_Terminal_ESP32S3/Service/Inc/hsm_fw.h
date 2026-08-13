#ifndef __HSM_FW_H_
#define __HSM_FW_H_

/**
 * @file hsm_fw.h
 * @brief 通用层次状态机框架（架构 3.10）
 *
 * 将 conn_hsm 上升为通用 HSM：状态树 + 跃迁表 + 进入/退出动作 + 历史态。
 * 适用于 OTA 状态机、双链路切换、蓝牙配对等复杂状态管理。
 *
 * 设计要点：
 *  - 状态节点含 parent（层次）、on_entry/on_exit 动作、跃迁表；
 *  - hsm_dispatch 按当前状态查找跃迁：本层优先，逐级向 parent 委托；
 *  - 跃迁时自动执行 on_exit（旧态）→ on_entry（新态）；
 *  - 可选历史态（HSM_HISTORY 子状态复位时回到上次子状态）。
 *
 * 线程安全：单任务内派发（状态机不重入）；如需跨任务，调用方加锁。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if HsmFwUse

/* 前向声明 */
typedef struct hsm_node hsm_node_t;
typedef struct hsm hsm_t;

/* 跃迁表项 */
typedef struct {
    uint16_t          event_id;    /* 触发事件 */
    const hsm_node_t *target;      /* 目标状态（NULL=由 parent 处理） */
} hsm_trans_t;

/* 状态节点 */
struct hsm_node {
    const char       *name;
    const hsm_node_t *parent;          /* 父状态（层次） */
    void (*on_entry)(hsm_t *me, void *payload, uint8_t len);
    void (*on_exit)(hsm_t *me);
    const hsm_trans_t *trans;           /* 本层跃迁表 */
    uint8_t           trans_cnt;
    bool              has_history;      /* 历史态标记 */
};

/* 状态机实例 */
struct hsm {
    const hsm_node_t *current;         /* 当前状态 */
    const hsm_node_t *history;         /* 历史态记录（用于复位） */
    void             *user;            /* 用户上下文 */
};

/* ===================== API ===================== */

/* 初始化：进入初始状态 */
void hsm_init(hsm_t *me, const hsm_node_t *init_state, void *user);

/* 派发事件：沿状态树向上查找跃迁，执行 on_exit/on_entry */
void hsm_dispatch(hsm_t *me, uint16_t event_id, void *payload, uint8_t len);

/* 强制切换状态（直接退出/进入，不走跃迁表） */
void hsm_transition(hsm_t *me, const hsm_node_t *target);

/* 当前状态名（诊断） */
const char *hsm_state_name(const hsm_t *me);

/* 重置到历史态（若根状态 has_history，则回到上次子状态） */
void hsm_reset_to_history(hsm_t *me);

/* init 表接入占位 */
void hsm_fw_init_stub(void *arg);

#endif /* HsmFwUse */

#endif /* __HSM_FW_H_ */