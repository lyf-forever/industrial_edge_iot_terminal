/**
 * @file soft_timer.c
 * @brief 软定时器轮实现：esp_timer 驱动三层时间轮
 *
 * 实现：一个 1ms 的 esp_timer 周期源驱动三层计数器；每层维护
 * 静态槽表（scheduled tick + cb）。到点回调。
 */

#include "soft_timer.h"

#if SoftTimerUse

#include "esp_timer.h"
#include "esp_log.h"
#include <string.h>

#define TIMER_1MS_PERIOD_US    1000
#define TIMER_10MS_PERIOD_US   10000
#define TIMER_100MS_PERIOD_US  100000

typedef struct {
    bool    used;
    uint32_t target;        /* 触发目标 tick */
    uint32_t period;        /* 周期（tick 数），0=单次 */
    void  (*cb)(void *);
    void   *user;
} timer_slot_t;

static timer_slot_t s_slots_1ms[SOFT_TIMER_MAX_1MS];
static timer_slot_t s_slots_10ms[SOFT_TIMER_MAX_10MS];
static timer_slot_t s_slots_100ms[SOFT_TIMER_MAX_100MS];

static volatile uint32_t s_tick[3] = {0, 0, 0};
static bool s_inited = false;

static timer_slot_t *slot_base(timer_wheel_t w)
{
    switch (w) {
        case TIMER_WHEEL_1MS:   return s_slots_1ms;
        case TIMER_WHEEL_10MS:  return s_slots_10ms;
        default:                return s_slots_100ms;
    }
}

static uint8_t slot_max(timer_wheel_t w)
{
    switch (w) {
        case TIMER_WHEEL_1MS:   return SOFT_TIMER_MAX_1MS;
        case TIMER_WHEEL_10MS:  return SOFT_TIMER_MAX_10MS;
        default:                return SOFT_TIMER_MAX_100MS;
    }
}

static void wheel_tick(timer_wheel_t w)
{
    s_tick[w]++;
    timer_slot_t *slots = slot_base(w);
    uint8_t max = slot_max(w);
    for (uint8_t i = 0; i < max; i++) {
        timer_slot_t *s = &slots[i];
        if (!s->used || s->target != s_tick[w]) continue;
        if (s->cb) s->cb(s->user);
        if (s->period) {
            s->target = s_tick[w] + s->period;   /* 重新安排 */
        } else {
            s->used = false;                     /* 单次：注销 */
        }
    }
}

static void timer1ms_cb(void *arg)
{
    (void)arg;
    wheel_tick(TIMER_WHEEL_1MS);
    if (s_tick[TIMER_WHEEL_1MS] % 10 == 0) wheel_tick(TIMER_WHEEL_10MS);
    if (s_tick[TIMER_WHEEL_1MS] % 100 == 0) wheel_tick(TIMER_WHEEL_100MS);
}

void soft_timer_init(void *arg)
{
    (void)arg;
    memset(s_slots_1ms, 0, sizeof(s_slots_1ms));
    memset(s_slots_10ms, 0, sizeof(s_slots_10ms));
    memset(s_slots_100ms, 0, sizeof(s_slots_100ms));

    const esp_timer_create_args_t args = {
        .callback = timer1ms_cb,
        .name = "soft_timer_wheel",
    };
    esp_timer_handle_t h = NULL;
    if (esp_timer_create(&args, &h) != ESP_OK) {
        ESP_LOGE("soft_timer", "esp_timer_create failed");
        return;
    }
    esp_timer_start_periodic(h, TIMER_1MS_PERIOD_US);
    s_inited = true;
    ESP_LOGI("soft_timer", "init ok");
}

static soft_timer_t find_free(timer_wheel_t w)
{
    timer_slot_t *slots = slot_base(w);
    uint8_t max = slot_max(w);
    for (uint8_t i = 0; i < max; i++) {
        if (!slots[i].used) return (soft_timer_t)i;
    }
    return -1;
}

soft_timer_t soft_timer_add(timer_wheel_t w, uint32_t period,
                            void (*cb)(void *), void *user)
{
    if (!s_inited || cb == NULL || period == 0 || w >= TIMER_WHEEL_MAX) return -1;
    soft_timer_t idx = find_free(w);
    if (idx < 0) return -1;
    timer_slot_t *s = &slot_base(w)[idx];
    s->used = true;
    s->period = period;
    s->target = s_tick[w] + period;   /* 首个触发点 */
    s->cb = cb;
    s->user = user;
    return idx;
}

void soft_timer_del(soft_timer_t t)
{
    if (t < 0) return;
    /* 简化：仅在 1ms 轮查找（句柄全局唯一分配于注册轮，
     * 此处按三段区间反查归属轮） */
    for (uint8_t w = 0; w < TIMER_WHEEL_MAX; w++) {
        if (t < slot_max((timer_wheel_t)w)) {
            timer_slot_t *s = &slot_base((timer_wheel_t)w)[t];
            if (s->used) { s->used = false; }
            return;
        }
        t -= slot_max((timer_wheel_t)w);
    }
}

uint32_t soft_timer_tick(timer_wheel_t w)
{
    if (w >= TIMER_WHEEL_MAX) return 0;
    return s_tick[w];
}

#endif /* SoftTimerUse */