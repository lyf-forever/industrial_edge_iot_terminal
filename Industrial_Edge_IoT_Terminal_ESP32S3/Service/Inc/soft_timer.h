#ifndef __SOFT_TIMER_H_
#define __SOFT_TIMER_H_

/**
 * @file soft_timer.h
 * @brief 软定时器轮（架构 3.8）
 *
 * 基于 esp_timer 的三层时间轮（1ms / 10ms / 100ms），
 * 面向"大量轻量短周期回调"场景，比每任务各自 vTaskDelay
 * 更省栈与上下文切换。对齐 GD32 端预留的 driver_tick_handle[] 思想。
 *
 * 注意：回调运行于 esp_timer 任务上下文，必须短小非阻塞；
 * 禁止在回调中调用会阻塞的 API（信号量等待等）。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if SoftTimerUse

/* 时间轮层次（tick 周期） */
typedef enum {
    TIMER_WHEEL_1MS = 0,     /* 1ms 轮 */
    TIMER_WHEEL_10MS,        /* 10ms 轮 */
    TIMER_WHEEL_100MS,       /* 100ms 轮 */
    TIMER_WHEEL_MAX
} timer_wheel_t;

/* 软定时器句柄 */
typedef int8_t soft_timer_t;   /* >=0 有效，-1 无效 */

/* 每轮最大槽数（1ms 轮按 1000 槽，10ms/100ms 轮各 100 槽） */
#define SOFT_TIMER_MAX_1MS    32
#define SOFT_TIMER_MAX_10MS   16
#define SOFT_TIMER_MAX_100MS  16

/* ===================== API ===================== */

/* 初始化软定时器轮（init 表 SOFTWARE 阶段，优先于其他服务） */
void soft_timer_init(void *arg);

/**
 * @brief 注册一个周期软定时器
 * @param wheel  所属轮层次
 * @param period 周期（单位=所在轮的 tick，如 10ms 轮 period=5 表示 50ms）
 * @param cb     回调
 * @param user   用户上下文
 * @return 句柄（-1 失败）
 */
soft_timer_t soft_timer_add(timer_wheel_t wheel, uint32_t period,
                            void (*cb)(void *user), void *user);

/* 注销软定时器 */
void soft_timer_del(soft_timer_t t);

/* 每轮当前 tick（调试用） */
uint32_t soft_timer_tick(timer_wheel_t wheel);

#endif /* SoftTimerUse */

#endif /* __SOFT_TIMER_H_ */