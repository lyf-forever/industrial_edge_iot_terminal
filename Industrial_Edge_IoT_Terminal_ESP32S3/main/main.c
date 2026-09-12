/**
 * @file main.c
 * @brief ESP32S3 无线通信端固件入口
 *
 * 基于分阶段初始化表(initTable) + 任务注册表(taskTable)的表驱动框架：
 *   app_main -> System_Module_Init(硬件/软件初始化表)
 *             -> System_Init(软件钩子)
 *             -> 创建 StartTask -> 创建 worker 任务 -> 删除自身
 *
 * 初始化表支持架构 3.1：depends_on 依赖位掩码 + 阶段内拓扑排序
 * （mod_registry），表项顺序可任意，依赖自动满足。
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
#if BleGattUse || TcpSrvUse
#include "net_bridge.h"
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
#if ModRegUse
#include "mod_registry.h"
#endif
#if MemPoolUse
#include "mempool.h"
#endif
#if SoftTimerUse
#include "soft_timer.h"
#endif
#if ChannelUse
#include "channel.h"
#endif
#if CmdUse
#include "cmd_dispatcher.h"
#endif
#if HsmFwUse
#include "hsm_fw.h"
#endif
#if LogUse
#include "log_fw.h"
#endif
#if TtsUse
#include "tts.h"
#endif
#if ActorUse
#include "actor.h"
#endif
#if CredUse
#include "cred_mgr.h"
#endif
#if OtaUse
#include "ota_mgr.h"
#endif
#if AiUse
#include "ai_pipeline.h"
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

#if ModRegUse
/* 架构 3.1：启用依赖拓扑排序 —— 表项用 MOD_INIT_ITEM（含 mod_id + depends_on） */
static const mod_init_item_t initTable[] = {
    /* ---- 硬件阶段 ---- */
#if SensorUse && Mq2Use
    MOD_INIT_ITEM("mq2",        INIT_STAGE_HARDWARE, mq2_drv_init,    NULL, 0, MOD_MAX),
#endif
#if Ws2812Use
    MOD_INIT_ITEM("ws2812",     INIT_STAGE_HARDWARE, ws2812_init,     NULL, 0, MOD_MAX),
#endif
#if LinkUse
    MOD_INIT_ITEM("bsp_uart",   INIT_STAGE_HARDWARE, bsp_uart_init,   NULL, 0, MOD_LINK),
#endif

    /* ---- 软件阶段 ---- */
#if MemPoolUse
    MOD_INIT_ITEM("mempool",    INIT_STAGE_SOFTWARE, mempool_init_stub, NULL, 0, MOD_MEMPOOL),
#endif
#if EventBusUse
    MOD_INIT_ITEM("event_bus",  INIT_STAGE_SOFTWARE, event_bus_init,  NULL, 0, MOD_EVENT_BUS),
#endif
#if SoftTimerUse
    MOD_INIT_ITEM("soft_timer", INIT_STAGE_SOFTWARE, soft_timer_init, NULL, 0, MOD_SOFT_TIMER),
#endif
#if TtsUse
    MOD_INIT_ITEM("tts",        INIT_STAGE_SOFTWARE, tts_init,        NULL, 0, MOD_TTS),
#endif
#if LogUse
    MOD_INIT_ITEM("log_fw",     INIT_STAGE_SOFTWARE, log_fw_init,     NULL, 0, MOD_LOG),
#endif
#if ChannelUse && LinkUse
    MOD_INIT_ITEM("channel",    INIT_STAGE_SOFTWARE, channel_init,    NULL, 0, MOD_CHANNEL),
#endif
#if CredUse
    MOD_INIT_ITEM("cred_mgr",   INIT_STAGE_SOFTWARE, cred_mgr_init,   NULL, 0, MOD_CRED),
#endif
#if EventBusUse && HsmFwUse
    MOD_INIT_ITEM("hsm_fw",     INIT_STAGE_SOFTWARE, hsm_fw_init_stub, NULL, BIT_MOD(MOD_EVENT_BUS), MOD_HSM_FW),
#endif
#if HsmUse
    /* conn_hsm 依赖事件总线 */
    MOD_INIT_ITEM("conn_hsm",   INIT_STAGE_SOFTWARE, conn_hsm_init,   NULL,
                  BIT_MOD(MOD_EVENT_BUS), MOD_HSM),
#endif
#if EventBusUse && LinkUse && ChannelUse
    /* event_ipc 依赖 事件总线 + 通道 + 链路 */
    MOD_INIT_ITEM("event_ipc",  INIT_STAGE_SOFTWARE, event_ipc_init,  NULL,
                  BIT_MOD(MOD_EVENT_BUS) | BIT_MOD(MOD_CHANNEL), MOD_MAX),
#endif
#if AiUse && EventBusUse
    MOD_INIT_ITEM("ai_pipe",    INIT_STAGE_SOFTWARE, ai_pipeline_init, NULL, BIT_MOD(MOD_EVENT_BUS), MOD_AI),
#endif
#if OtaUse && HsmFwUse
    MOD_INIT_ITEM("ota_mgr",    INIT_STAGE_SOFTWARE, ota_mgr_init,    NULL,
                  BIT_MOD(MOD_HSM_FW), MOD_OTA),
#endif
#if CmdUse
    MOD_INIT_ITEM("cmd_disp",   INIT_STAGE_SOFTWARE, cmd_disp_init,   NULL, 0, MOD_CMD),
#endif
#if WiFiUse
    MOD_INIT_ITEM("wifi_mgr",   INIT_STAGE_SOFTWARE, wifi_manager_init, NULL, 0, MOD_WIFI),
#endif
#if MqttUse
    MOD_INIT_ITEM("mqtt_app",   INIT_STAGE_SOFTWARE, mqtt_client_app_init, NULL,
                  BIT_MOD(MOD_WIFI), MOD_MQTT),
#endif
#if CloudUse
    MOD_INIT_ITEM("cloud_br",   INIT_STAGE_SOFTWARE, cloud_bridge_init, NULL,
                  BIT_MOD(MOD_EVENT_BUS) | BIT_MOD(MOD_MQTT), MOD_CLOUD),
#endif
#if (BleGattUse || TcpSrvUse) && EventBusUse && LinkUse
    /* 无线透传桥（BLE NUS + TCP Server）：依赖事件总线/通道（经 event_ipc 喂包/广播） */
    MOD_INIT_ITEM("net_bridge", INIT_STAGE_SOFTWARE, net_bridge_init, NULL,
                  BIT_MOD(MOD_EVENT_BUS) | BIT_MOD(MOD_CHANNEL), MOD_MAX),
#endif
};
#else
/* 传统 initItem 表（ModRegUse=0 时回退） */
typedef struct {
    const char     *name;
    initStage       stage;
    initInterface   init_func;
    void           *arg;
    uint32_t        depends_on;
} initItem;

#define INIT_ITEM(_name, _stage, _func, _arg, _dep)  \
    { .name = (_name), .stage = (_stage), .init_func = (_func), \
      .arg = (_arg), .depends_on = (_dep) }

static const initItem initTable[] = {
#if SensorUse && Mq2Use
    INIT_ITEM("mq2",        INIT_STAGE_HARDWARE, mq2_drv_init,    NULL, 0),
#endif
#if Ws2812Use
    INIT_ITEM("ws2812",     INIT_STAGE_HARDWARE, ws2812_init,     NULL, 0),
#endif
#if LinkUse
    INIT_ITEM("bsp_uart",   INIT_STAGE_HARDWARE, bsp_uart_init,   NULL, 0),
#endif
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
#if (BleGattUse || TcpSrvUse) && EventBusUse && LinkUse
    INIT_ITEM("net_bridge", INIT_STAGE_SOFTWARE, net_bridge_init, NULL, 0),
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
#endif /* ModRegUse */

/* 应用配置：包括模块片上外设初始化、软件初始化 */
static inline void System_Module_Init(void)
{
#define INIT_TABLE_SIZE  (sizeof(initTable) / sizeof(initTable[0]))
#if ModRegUse
    uint16_t hw = mod_registry_run_stage(initTable, INIT_TABLE_SIZE, INIT_STAGE_HARDWARE);
    uint16_t sw = mod_registry_run_stage(initTable, INIT_TABLE_SIZE, INIT_STAGE_SOFTWARE);
    ESP_LOGI(TAG, "init done: hw=%u sw=%u", hw, sw);
    mod_registry_dump(initTable, INIT_TABLE_SIZE, INIT_STAGE_SOFTWARE);
#else
    initTable_run(initTable, INIT_TABLE_SIZE);
#endif
}

void app_main(void)
{
    /* 1. 硬件 + 软件分阶段初始化（init 表，含依赖拓扑） */
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