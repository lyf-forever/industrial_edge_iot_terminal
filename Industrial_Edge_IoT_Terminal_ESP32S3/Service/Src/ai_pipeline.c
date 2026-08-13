/**
 * @file ai_pipeline.c
 * @brief 边缘 AI 推理管线实现：滑动窗口统计异常检测
 */

#include "ai_pipeline.h"

#if AiUse

#include "event_bus.h"
#include "event_id.h"
#include "link_payload.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>

static const char *TAG = "ai";

#define AI_CHANNELS   3            /* gas/temp/humid */
#define AI_WINDOW     16           /* 窗口样本数 */
#define AI_FILTER_ALPHA 0.3f       /* 一阶低通滤波系数（平滑） */

typedef struct {
    int16_t  buf[AI_WINDOW];
    uint8_t  idx;
    uint8_t  filled;
    float    filtered;      /* 一阶低通滤波输出（平滑值） */
    float    prev_value;    /* 上次滤波值（趋势斜率用） */
} channel_state_t;

static channel_state_t s_ch[AI_CHANNELS];
static ai_feature_t    s_feat[AI_CHANNELS];

static void compute_feature(uint8_t ch)
{
    channel_state_t *s = &s_ch[ch];
    ai_feature_t *f = &s_feat[ch];
    int n = s->filled < AI_WINDOW ? s->filled : AI_WINDOW;
    if (n == 0) { memset(f, 0, sizeof(*f)); return; }

    float sum = 0;
    float maxv = -1e9f;
    for (int i = 0; i < n; i++) {
        sum += s->buf[i];
        if (s->buf[i] > maxv) maxv = s->buf[i];
    }
    f->mean = sum / n;
    float var = 0;
    for (int i = 0; i < n; i++) {
        float d = s->buf[i] - f->mean;
        var += d * d;
    }
    f->stddev = sqrtf(var / n);
    f->max = maxv;
    f->last = s->buf[(s->idx - 1 + AI_WINDOW) % AI_WINDOW];
}

/* 轻量异常评分：偏离均值超过 k*sigma 或超上限；附加趋势修正 */
static uint8_t score_anomaly(const ai_feature_t *f, float upper_limit, int16_t cur)
{
    if (cur > upper_limit) return 2;                       /* 硬超限=异常 */
    float dev = fabsf((float)cur - f->mean);
    if (f->stddev > 1e-6f && dev > 3.0f * f->stddev) return 2;  /* 3σ */
    if (f->stddev > 1e-6f && dev > 2.0f * f->stddev) return 1;  /* 2σ 预警 */
    return 0;
}

/* 趋势检测：连续窗口内滤波值单调上升超过阈值则视为上升趋势（预警信号） */
static uint8_t trend_rising(uint8_t ch, int16_t cur)
{
    channel_state_t *s = &s_ch[ch];
    float df = (float)cur - s->prev_value;   /* 单步增量 */
    s->prev_value = (float)cur;
    /* 连续 3 步同向上升（|df|>1.0）判定为趋势 */
    static uint8_t streak[AI_CHANNELS] = {0};
    if (df > 1.0f) {
        if (++streak[ch] >= 3) return 1;
    } else {
        streak[ch] = 0;
    }
    return 0;
}

static void publish_anomaly(uint8_t ch, int16_t cur)
{
    ai_anomaly_t a = {0};
    a.channel = ch;
    a.value = cur;
    /* 各通道上限（x10）：gas 10000ppm=100000? 用相对阈值 */
    float limit = (ch == 0) ? 10000.0f : (ch == 1 ? 800.0f : 900.0f);
    a.level = score_anomaly(&s_feat[ch], limit, cur);

    /* 趋势修正：正常范围内但持续上升 → 预警（早于 σ 报警） */
    if (a.level == 0 && trend_rising(ch, cur)) {
        a.level = 1;
    }

    float dev = fabsf((float)cur - s_feat[ch].mean);
    a.score = (uint16_t)(dev * 1000.0f / (s_feat[ch].stddev + 1e-6f));
    if (a.score > 1000) a.score = 1000;

    /* 多通道联动：若 gas 预警且温度同时预警 → 升级为异常（联动评分） */
    if (a.level == 1 && ch == 0) {
        if (s_feat[1].last > 0 && s_feat[2].last > 0) {
            /* 简单联动规则：高温+高湿+气体上升 → 疑似燃烧 */
            if (s_feat[1].last > 700.0f && s_feat[2].last > 800.0f) {
                a.level = 2;
                a.score = 1000;
            }
        }
    }

    /* 异常事件仅本地（不上云高频），由 cloud_bridge 订阅后按 level 上云 */
    event_bus_publish_local(EVT_SENSOR_ANOMALY, (const uint8_t *)&a, sizeof(a));
}

void ai_pipeline_feed(uint8_t channel, int16_t value)
{
    if (channel >= AI_CHANNELS) return;
    channel_state_t *s = &s_ch[channel];
    /* 一阶低通滤波（平滑瞬时抖动） */
    s->filtered = (s->filled == 0)
        ? (float)value
        : AI_FILTER_ALPHA * (float)value + (1.0f - AI_FILTER_ALPHA) * s->filtered;

    s->buf[s->idx] = (int16_t)s->filtered;   /* 窗口存滤波值（降噪） */
    s->idx = (s->idx + 1) % AI_WINDOW;
    if (s->filled < AI_WINDOW) s->filled++;
    compute_feature(channel);
    publish_anomaly(channel, (int16_t)s->filtered);
}

static void on_sensor_event(uint16_t event_id, const uint8_t *payload,
                            uint8_t len, uint8_t source, void *user_data)
{
    (void)event_id; (void)len; (void)source; (void)user_data;
    if (payload == NULL) return;
    const sensor_payload_t *sp = (const sensor_payload_t *)payload;
    ai_pipeline_feed(0, (int16_t)sp->gas_ppm);
    ai_pipeline_feed(1, sp->temp_c);
    ai_pipeline_feed(2, (int16_t)sp->humid_pct);
}

void ai_pipeline_init(void *arg)
{
    (void)arg;
    memset(s_ch, 0, sizeof(s_ch));
    memset(s_feat, 0, sizeof(s_feat));
    event_bus_subscribe(EVT_SENSOR_DATA, on_sensor_event, NULL);
    ESP_LOGI(TAG, "init ok (window=%d ch=%d)", AI_WINDOW, AI_CHANNELS);
}

const ai_feature_t *ai_pipeline_feature(uint8_t channel)
{
    if (channel >= AI_CHANNELS) return NULL;
    return &s_feat[channel];
}

#endif /* AiUse */