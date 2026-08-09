/**
 * @file main.c
 * @brief ESP32S3 无线通信端固件入口
 *
 * 基于分阶段初始化表(initTable) + 任务注册表(taskTable)的表驱动框架：
 *   app_main -> System_Module_Init(硬件/软件初始化表)
 *             -> System_Init(软件钩子)
 *             -> 创建 StartTask -> 创建 worker 任务 -> 删除自身
 *
 * 初始化表通过 System/Inc/sys.h 中的功能开关宏来条件编译各模块。
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sys.h"
#include "app_tasks.h"

#if SensorUse && Mq2Use
#include "bsp_mq2.h"
#endif
#if Ws2812Use
#include "bsp_ws2812.h"
#endif
#if LinkUse
#include "bsp_uart.h"
#endif
#if WiFiUse
#include "wifi_manager.h"
#endif
#if MqttUse
#include "mqtt_client_app.h"
#endif
#if CloudUse
#include "cloud_bridge.h"
#endif
#if EventBusUse
#include "event_bus.h"
#if LinkUse
#include "event_ipc.h"
#endif
#endif
#if HsmUse
#include "conn_hsm.h"
#endif

static const char *TAG = "main";

/* StartTask 句柄存储 */
static TaskHandle_t xStartTaskHandle = NULL;

/* ===================== 初始化表框架 ===================== */

/* 初始化函数指针类型：返回 void，可传入用户参数 */
typedef void (*initInterface)(void *arg);

/* 初始化阶段枚举 */
typedef enum {
    INIT_STAGE_HARDWARE = 0,   /* 硬件初始化阶段（先执行） */
    INIT_STAGE_SOFTWARE,       /* 软件初始化阶段（后执行） */
    INIT_STAGE_MAX
} initStage;

/* 初始化表项：统一的描述结构体 */
typedef struct {
    const char     *name;        /* 步骤名称，用于日志或调试 */
    initStage       stage;       /* 所属初始化阶段 */
    initInterface   init_func;   /* 初始化函数指针 */
    void           *arg;         /* 传递给初始化函数的参数 */
    uint32_t        depends_on;  /* 依赖标记（位掩码或序号），0 表示无依赖 */
} initItem;

/* 提供宏来简化表项定义 */
#define INIT_ITEM(_name, _stage, _func, _arg, _dep)  \
    { .name = (_name), .stage = (_stage), .init_func = (_func), \
      .arg = (_arg), .depends_on = (_dep) }

/* ===================== 静态初始化表 ===================== *
 * 硬件阶段：板级外设驱动初始化
 * 软件阶段：服务层初始化（依赖硬件就绪）
 * */
static const initItem initTable[] = {
    /* ---- 硬件阶段 ---- */
#if SensorUse && Mq2Use
    INIT_ITEM("mq2",        INIT_STAGE_HARDWARE, mq2_drv_init,    NULL, 0),
#endif
#if Ws2812Use
    INIT_ITEM("ws2812",     INIT_STAGE_HARDWARE, ws2812_init,     NULL, 0),
#endif
#if LinkUse
    INIT_ITEM("bsp_uart",   INIT_STAGE_HARDWARE, bsp_uart_init,   NULL, 0),
#endif

    /* ---- 软件阶段 ---- */
#if EventBusUse
    INIT_ITEM("event_bus",  INIT_STAGE_SOFTWARE, event_bus_init,  NULL, 0),
#endif
#if HsmUse
    INIT_ITEM("conn_hsm",   INIT_STAGE_SOFTWARE, conn_hsm_init,   NULL, 0),
#endif
#if EventBusUse && LinkUse
    INIT_ITEM("event_ipc",  INIT_STAGE_SOFTWARE, event_ipc_init,  NULL, 0),
#endif
#if WiFiUse
    INIT_ITEM("wifi_mgr",   INIT_STAGE_SOFTWARE, wifi_manager_init, NULL, 0),
#endif
#if MqttUse
    INIT_ITEM("mqtt_app",   INIT_STAGE_SOFTWARE, mqtt_client_app_init, NULL, 0),
#endif
#if CloudUse
    INIT_ITEM("cloud_br",   INIT_STAGE_SOFTWARE, cloud_bridge_init, NULL, 0),
#endif
};

/* 初始化调度器：按阶段顺序执行表内所有项 */
static void initTable_run(const initItem *table, size_t count)
{
    for (initStage stage = INIT_STAGE_HARDWARE; stage < INIT_STAGE_MAX; stage++)
    {
        for (size_t i = 0; i < count; i++)
        {
            if (table[i].stage == stage) {
                ESP_LOGI(TAG, "Running [%s]...", table[i].name);
                if (table[i].init_func) {
                    table[i].init_func(table[i].arg);
                }
                ESP_LOGI(TAG, "[%s] init finished", table[i].name);
            }
        }
    }
}

/* 应用配置：包括模块片上外设初始化、软件初始化 */
static inline void System_Module_Init(void)
{
    initTable_run(initTable, sizeof(initTable) / sizeof(initTable[0]));
}

void app_main(void)
{
    /* 1. 硬件 + 软件分阶段初始化（init 表） */
    System_Module_Init();

    /* 2. 系统级软件初始化钩子 */
    System_Init();

    /* 3. 创建 StartTask：StartTask 创建全部 worker 任务后删除自身 */
    BaseType_t ret = xTaskCreatePinnedToCore(StartTask, "startTask",
                                             TASK_START_STK_SIZE, NULL,
                                             TASK_START_PRIO, &xStartTaskHandle,
                                             0);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create StartTask");
        return;
    }

    /* app_main 返回后 ESP-IDF 自动删除 main task */
}