/**
 * @file app_tasks.c
 * @brief 应用任务注册表与实现
 *
 * 设计要点（修复原版 bug 并扩展）：
 *  1. taskTable 仅包含 worker 任务（不含 StartTask），StartTask 创建
 *     taskTable 中全部任务后删除自身，避免原版递归重建 StartTask 的 bug；
 *  2. App_Tasks_Create() 无参版本供 main.c 调用，解决原版
 *     createTasks_fromTable() 参数缺失的编译/链接问题；
 *  3. createTasks_fromTable 内部使用正确的结构体字段名(taskFunc/coreID 等)。
 */

#include "app_tasks.h"

#if SensorUse && Mq2Use
#include "bsp_mq2.h"
#endif
#if LinkUse
#include "bsp_uart.h"
#endif
#if CloudUse
#include "cloud_bridge.h"
#endif
#if EventBusUse && LinkUse
#include "event_ipc.h"
#endif
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "app_tasks";

/* ===================== 任务句柄 ===================== */
#if SensorUse
static TaskHandle_t xSensorTaskHandle  = NULL;
#endif
#if LinkUse
static TaskHandle_t xLinkRxTaskHandle  = NULL;
#endif
#if CloudUse
static TaskHandle_t xCloudTaskHandle   = NULL;
#endif
#if EventBusUse && LinkUse
static TaskHandle_t xEventIpcTaskHandle = NULL;
#endif

/* 系统初始化（任务创建前的软件初始化钩子） */
void System_Init(void)
{
    /* 当前为空，可在该处添加非 init 表覆盖的软件初始化逻辑 */
}

/* ===================== 任务注册表 ===================== *
 * 仅 worker 任务，StartTask 负责创建这些任务。
 * */
static const taskItem taskTable[] = {
#if LinkUse
    TASK_ITEM(bsp_uart_rx_task, "linkRxTask", TASK_LINK_RX_STK_SIZE,
              NULL, TASK_LINK_RX_PRIO, 0, &xLinkRxTaskHandle),
#endif
#if EventBusUse && LinkUse
    TASK_ITEM(event_ipc_task, "eventIpcTask", TASK_EVENT_IPC_STK_SIZE,
              NULL, TASK_EVENT_IPC_PRIO, tskNO_AFFINITY, &xEventIpcTaskHandle),
#endif
#if CloudUse
    TASK_ITEM(cloud_bridge_task, "cloudTask", TASK_CLOUD_STK_SIZE,
              NULL, TASK_CLOUD_PRIO, tskNO_AFFINITY, &xCloudTaskHandle),
#endif
#if SensorUse
    TASK_ITEM(SensorTask, "sensorTask", TASK_SENSOR_STK_SIZE,
              NULL, TASK_SENSOR_PRIO, 1, &xSensorTaskHandle),
#endif
};

static size_t task_getItemSize(void)
{
    return sizeof(taskTable) / sizeof(taskTable[0]);
}

/**
 * @brief 根据任务注册表批量创建任务
 * @param table  任务信息数组
 * @return 成功创建的任务数量
 */
static uint16_t createTasks_fromTable(const taskItem *table)
{
    uint16_t created = 0;
    size_t allCounts = task_getItemSize();

    for (size_t i = 0; i < allCounts; i++) {
        const taskItem *item = &table[i];
        TaskHandle_t *handle_ptr = item->handle;
        BaseType_t ret;

        if (item->coreID == tskNO_AFFINITY) {
            ret = xTaskCreate(item->taskFunc, item->name,
                              item->stackDepth, item->param,
                              item->priority, handle_ptr);
        } else {
            ret = xTaskCreatePinnedToCore(item->taskFunc, item->name,
                                          item->stackDepth, item->param,
                                          item->priority, handle_ptr,
                                          item->coreID);
        }

        if (ret != pdPASS) {
            ESP_LOGE(TAG, "Failed to create task [%s]", item->name);
        } else {
            ESP_LOGI(TAG, "[%s] create OK", item->name);
            created++;
        }
    }
    return created;
}

/* 无参封装：供 main.c 调用 */
void App_Tasks_Create(void)
{
    uint16_t created = createTasks_fromTable(taskTable);
    ESP_LOGI(TAG, "Created %u worker tasks", created);
}

/**
 * @brief 开始任务：创建注册表内全部 worker 任务后删除自身
 */
void StartTask(void *pvParameters)
{
    (void)pvParameters;

    /* worker 任务已在 taskTable 静态声明，直接批量创建 */
    taskENTER_CRITICAL();
    App_Tasks_Create();
    taskEXIT_CRITICAL();

    ESP_LOGI(TAG, "StartTask done, deleting self");
    vTaskDelete(NULL);
}

/* ===================== 传感器任务 ===================== */
#if SensorUse
#if Mq2Use
#if EventBusUse
#include "event_bus.h"
#include "event_id.h"
#include "link_payload.h"
#endif
void SensorTask(void *pvParameters)
{
    (void)pvParameters;
    while (1) {
        float ppm = mq2_getConcentration();
        uint16_t pct = mq2_getPercentage();
        uint32_t ppm_u = (uint32_t)ppm;
        ESP_LOGI("sensor", "MQ2 ppm=%lu pct=%u", (unsigned long)ppm_u, pct);

#if EventBusUse
        /* 将本端采集数据以事件形式投递总线：
         * - cloud_bridge 订阅该事件 -> JSON 上报 MQTT
         * - event_ipc 自动跨核转发 -> GD32H7 显示
         * 采集模块无需关心下游谁处理。 */
        sensor_payload_t sp = {0};
        sp.temp_c = 0;
        sp.humid_pct = 0;
        sp.gas_ppm = (uint16_t)ppm_u;
        sp.co2_ppm = 0;
        sp.pressure_hpa = 0;
        event_bus_publish_any(EVT_SENSOR_DATA, (const uint8_t *)&sp, sizeof(sp));

        /* 越限告警事件 */
        if (ppm_u > 1000) {
            alarm_payload_t ap = {0};
            ap.alarm_id = 1;
            ap.alarm_level = (ppm_u > 2000) ? 2 : 1;
            ap.sensor_val = (uint16_t)ppm_u;
            event_bus_publish_any(EVT_SENSOR_ALARM, (const uint8_t *)&ap, sizeof(ap));
        }
#else
        if (ppm_u > 1000) {
            ESP_LOGW("sensor", "Gas high! ppm=%lu", (unsigned long)ppm_u);
        }
#endif
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
#else
void SensorTask(void *pvParameters) { (void)pvParameters; vTaskDelete(NULL); }
#endif
#endif

/* WS2812 状态指示已由 conn_hsm(连接层次状态机)驱动：
 * HSM 在通信状态跃迁时直接调用 ws2812_writeGRB 设置状态色，
 * 无需单独的 WS2812 任务，符合"状态机驱动指示"的解耦设计。 */

/* EventIpcTask 的实际实现见 Service/Src/event_ipc.c 的 event_ipc_task() */