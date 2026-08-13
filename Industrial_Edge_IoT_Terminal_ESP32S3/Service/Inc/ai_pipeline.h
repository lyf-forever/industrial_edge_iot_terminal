#ifndef __AI_PIPELINE_H_
#define __AI_PIPELINE_H_

/**
 * @file ai_pipeline.h
 * @brief 边缘 AI 推理管线（架构 3.13）
 *
 * 基于 ESP32S3 向量指令的轻量端侧推理：订阅 EVT_SENSOR_DATA，
 * 提取特征 → 轻量异常检测（滑动窗口均值/方差 + 阈值）→
 * 发布 EVT_SENSOR_ANOMALY / EVT_KWS 事件，供 conn_hsm 触发告警、
 * cloud_bridge 上云。体现"端侧智能、按需上云"。
 *
 * 说明：本实现为可运行的轻量异常检测骨架（无外部模型文件），
 * 后续可替换为量化的 TinyML 模型推理函数。
 */

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#if AiUse

/* 特征向量 */
typedef struct {
    float   mean;      /* 窗口均值 */
    float   stddev;    /* 窗口标准差 */
    float   max;       /* 窗口最大值 */
    float   last;      /* 最新值 */
} ai_feature_t;

/* 异常结果（发布到事件总线 EVT_SENSOR_ANOMALY 的 payload） */
typedef struct {
    uint8_t  channel;   /* 0=gas 1=temp 2=humid */
    uint8_t  level;     /* 0=正常 1=预警 2=异常 */
    uint16_t score;     /* 异常分数 0-1000 */
    int16_t  value;     /* 当前值 */
} ai_anomaly_t;

/* 唤醒词命中结果（EVT_KWS 的 payload） */
typedef struct {
    uint8_t  keyword_id;
    uint8_t  confidence;   /* 0-100 */
} ai_kws_t;

/* ===================== API ===================== */

/* 初始化推理管线（订阅事件总线），SOFTWARE 阶段 */
void ai_pipeline_init(void *arg);

/* 手动喂入一个传感器样本（单位 x10，如 256=25.6） */
void ai_pipeline_feed(uint8_t channel, int16_t value);

/* 当前特征（诊断） */
const ai_feature_t *ai_pipeline_feature(uint8_t channel);

#endif /* AiUse */

#endif /* __AI_PIPELINE_H_ */