#ifndef __ACTOR_H_
#define __ACTOR_H_

/**
 * @file actor.h
 * @brief Actor 模型（架构 3.4）
 *
 * 每个 Actor 拥有独立任务 + 输入队列，仅通过消息通信，无共享状态。
 * 适合长耗时且需状态隔离的处理（OTA 写盘、边缘 AI 推理、日志落盘）。
 * 消息对象建议从对象池（mempool）分配，处理完毕后归还。
 *
 * 看门狗：可选的 wdt 秒数；>0 时内部注册 esp_task_wdt 并在每轮
 * 消息处理完喂狗；超时由 WDT 复位。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#if ActorUse

/* 消息：头部 + 负载指针（负载由调用方管理生命周期） */
typedef struct {
    uint16_t   msg_id;      /* 消息类型 */
    void      *payload;     /* 负载指针（可 NULL） */
    uint16_t   payload_len;
} actor_msg_t;

/* Actor 消息处理回调 */
typedef void (*actor_handler_t)(void *ctx, const actor_msg_t *msg);

/* Actor 描述 */
typedef struct {
    const char      *name;
    actor_handler_t handler;      /* 消息处理 */
    void            *ctx;         /* 处理回调上下文 */
    uint32_t         queue_len;   /* 队列深度 */
    uint32_t         stack_bytes;
    UBaseType_t      priority;
    BaseType_t       core;        /* 核亲和 */
    uint32_t         wdt_secs;    /* 0=不用 WDT */
} actor_desc_t;

/* Actor 句柄 */
typedef struct actor *actor_handle_t;

/* ===================== API ===================== */

/* 创建 Actor（启动任务），失败返回 NULL */
actor_handle_t actor_create(const actor_desc_t *desc);

/* 投递消息（非阻塞，队列满返回 false） */
bool actor_post(actor_handle_t a, uint16_t msg_id, void *payload, uint16_t len);

/* 投递消息（阻塞版，带超时） */
bool actor_post_timeout(actor_handle_t a, uint16_t msg_id,
                        void *payload, uint16_t len, uint32_t timeout_ms);

/* 查询队列使用量 */
uint32_t actor_queue_pending(actor_handle_t a);

/* 注销 Actor */
void actor_destroy(actor_handle_t a);

#endif /* ActorUse */

#endif /* __ACTOR_H_ */