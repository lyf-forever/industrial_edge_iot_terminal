/**
 * @file actor.c
 * @brief Actor 模型实现：独立任务 + 输入队列
 */

#include "actor.h"

#if ActorUse

#include "esp_log.h"
#include "esp_task_wdt.h"
#include <string.h>
#include <stdlib.h>

#define TAG "actor"

struct actor {
    actor_desc_t  desc;
    QueueHandle_t queue;
    TaskHandle_t  task;
    bool          running;
};

static void actor_task_entry(void *arg)
{
    actor_handle_t a = (actor_handle_t)arg;
    actor_msg_t msg;

    if (a->desc.wdt_secs > 0) {
        /* v2.0：检查注册结果，失败仅告警不中断（失去监督但业务可用） */
        esp_err_t wdt_err = esp_task_wdt_add(NULL);
        if (wdt_err != ESP_OK) {
            ESP_LOGW(TAG, "task wdt add failed: %s", esp_err_to_name(wdt_err));
        }
    }
    while (a->running) {
        if (xQueueReceive(a->queue, &msg, portMAX_DELAY)) {
            if (a->desc.handler) {
                a->desc.handler(a->desc.ctx, &msg);
            }
            if (a->desc.wdt_secs > 0) {
                esp_task_wdt_reset();
            }
        }
    }
    if (a->desc.wdt_secs > 0) {
        esp_task_wdt_delete(NULL);
    }
    vTaskDelete(NULL);
}

actor_handle_t actor_create(const actor_desc_t *desc)
{
    if (desc == NULL || desc->handler == NULL) return NULL;

    actor_handle_t a = calloc(1, sizeof(*a));
    if (a == NULL) return NULL;
    a->desc = *desc;
    a->running = true;

    a->queue = xQueueCreate(desc->queue_len ? desc->queue_len : 8,
                            sizeof(actor_msg_t));
    if (a->queue == NULL) {
        free(a);
        return NULL;
    }

    BaseType_t ret = xTaskCreatePinnedToCore(actor_task_entry, desc->name,
                                             desc->stack_bytes, a,
                                             desc->priority, &a->task,
                                             desc->core);
    if (ret != pdPASS) {
        vQueueDelete(a->queue);
        free(a);
        return NULL;
    }
    ESP_LOGI("actor", "[%s] created", desc->name);
    return a;
}

bool actor_post(actor_handle_t a, uint16_t msg_id, void *payload, uint16_t len)
{
    if (a == NULL || !a->running) return false;
    actor_msg_t msg = { .msg_id = msg_id, .payload = payload, .payload_len = len };
    return xQueueSend(a->queue, &msg, 0) == pdPASS;
}

bool actor_post_timeout(actor_handle_t a, uint16_t msg_id,
                        void *payload, uint16_t len, uint32_t timeout_ms)
{
    if (a == NULL || !a->running) return false;
    actor_msg_t msg = { .msg_id = msg_id, .payload = payload, .payload_len = len };
    return xQueueSend(a->queue, &msg, pdMS_TO_TICKS(timeout_ms)) == pdPASS;
}

uint32_t actor_queue_pending(actor_handle_t a)
{
    if (a == NULL || a->queue == NULL) return 0;
    return (uint32_t)uxQueueMessagesWaiting(a->queue);
}

void actor_destroy(actor_handle_t a)
{
    if (a == NULL) return;
    a->running = false;
    vTaskDelete(a->task);
    vQueueDelete(a->queue);
    free(a);
}

#endif /* ActorUse */