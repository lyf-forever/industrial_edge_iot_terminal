#ifndef __TTS_H_
#define __TTS_H_

/**
 * @file tts.h
 * @brief 时间触发调度器 TTS（架构 3.5）
 *
 * 基于 esp_timer 1kHz 硬件 tick 的确定性时间表调度：
 * 每个周期任务注册 (period_ticks, phase, fn)，在定时器 ISR 上下文
 * 按 tick 槽位分发，无抢占抖动。Cooperative 调度。
 *
 * 约束：回调运行于 esp_timer 任务上下文，必须短小非阻塞（<50us），
 * 重活请转交 Actor/任务处理。传感器采样/心跳/链路检测适合 TTS。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if TtsUse

#define TTS_SLOT_MAX   8

/* 时间表槽位 */
typedef struct {
    uint32_t   period_ticks;   /* 周期（tick 数，1 tick=1ms） */
    uint32_t   phase;          /* 相位偏移（0..period-1） */
    void     (*fn)(void *user);
    void      *user;
} tts_slot_t;

/* ===================== API ===================== */

/* 初始化 TTS（启动 1kHz tick），SOFTWARE 阶段 */
void tts_init(void *arg);

/* 注册时间表槽位，返回 0=成功 -1=表满/参数错 */
int8_t tts_register(const tts_slot_t *slot);

/* 注销槽位（按 fn 匹配） */
void tts_unregister(void (*fn)(void *));

/* 当前 tick 计数（诊断） */
uint32_t tts_tick_now(void);

#endif /* TtsUse */

#endif /* __TTS_H_ */