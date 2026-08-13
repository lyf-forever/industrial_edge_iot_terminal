/**
 * @file tts.c
 * @brief 时间触发调度器实现：esp_timer 1kHz tick 驱动时间表
 */

#include "tts.h"

#if TtsUse

#include "esp_timer.h"
#include "esp_log.h"
#include <string.h>

#define TTS_TICK_PERIOD_US  1000

typedef struct {
    bool        used;
    uint32_t    period_ticks;
    uint32_t    next_tick;    /* 下一个触发 tick */
    void      (*fn)(void *);
    void       *user;
} tts_internal_t;

static tts_internal_t s_slots[TTS_SLOT_MAX];
static volatile uint32_t s_tick = 0;
static bool s_inited = false;

static void tts_tick_cb(void *arg)
{
    (void)arg;
    s_tick++;
    for (int i = 0; i < TTS_SLOT_MAX; i++) {
        tts_internal_t *s = &s_slots[i];
        if (!s->used || s->next_tick != s_tick) continue;
        if (s->fn) s->fn(s->user);
        s->next_tick += s->period_ticks;   /* 重排下一触发点 */
    }
}

void tts_init(void *arg)
{
    (void)arg;
    memset(s_slots, 0, sizeof(s_slots));
    const esp_timer_create_args_t args = {
        .callback = tts_tick_cb,
        .name = "tts_tick",
    };
    esp_timer_handle_t h = NULL;
    if (esp_timer_create(&args, &h) != ESP_OK) {
        ESP_LOGE("tts", "create timer failed");
        return;
    }
    esp_timer_start_periodic(h, TTS_TICK_PERIOD_US);
    s_inited = true;
    ESP_LOGI("tts", "init ok");
}

int8_t tts_register(const tts_slot_t *slot)
{
    if (!s_inited || slot == NULL || slot->fn == NULL || slot->period_ticks == 0) {
        return -1;
    }
    for (int i = 0; i < TTS_SLOT_MAX; i++) {
        if (!s_slots[i].used) {
            s_slots[i].used = true;
            s_slots[i].period_ticks = slot->period_ticks;
            s_slots[i].next_tick = s_tick + slot->phase % slot->period_ticks + 1;
            s_slots[i].fn = slot->fn;
            s_slots[i].user = slot->user;
            return 0;
        }
    }
    return -1;
}

void tts_unregister(void (*fn)(void *))
{
    for (int i = 0; i < TTS_SLOT_MAX; i++) {
        if (s_slots[i].used && s_slots[i].fn == fn) {
            s_slots[i].used = false;
            return;
        }
    }
}

uint32_t tts_tick_now(void) { return s_tick; }

#endif /* TtsUse */