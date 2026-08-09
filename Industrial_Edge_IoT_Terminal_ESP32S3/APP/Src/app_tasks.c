#include "app_tasks.h"


/* 任务句柄 */
static TaskHandle_t xStartTaskHandle = NULL; // 开始任务
#if SensorUse
static TaskHandle_t xSensorTaskHandle = NULL; // 传感器采集任务
#endif 

/* 系统初始化 */
void System_Init(void)
{

}

/* 静态句柄变量声明 */
static TaskHandle_t xMq2TaskHandle  = NULL;

/* 任务注册表内已有的任务item数 */
static uint16_t taskTable_itemCnt = 1;  // 初始已有开始任务
/* 任务注册表 */
static const taskItem taskTable[] = {
    TASK_ITEM(StartTask, "startTask", TASK_START_STK_SIZE, NULL, TASK_START_PRIO, 0, &xStartTaskHandle),
    /* ......其他rtos任务放在StartTask内注册创建 */
};

static size_t task_getItemSize(void)
{
    return sizeof(taskTable) / sizeof(taskTable[0]);
}

static size_t taskItem_register()
{

}

/**
 * @brief  根据任务注册表批量创建任务
 * @param  table  任务信息数组
 * @param  count  任务数量
 * @return 成功创建的任务数量；若有失败，返回值可能小于 count
 */
uint16_t createTasks_fromTable(const taskItem *table)
{
    uint16_t created = 0;
    size_t allCounts = task_getItemSize(); 
    for (size_t i = 0; i < allCounts; i++) {
        const taskItem *item = &table[i];
        TaskHandle_t *handle_ptr = item->handle;

        BaseType_t ret;
        if (item->core_id == tskNO_AFFINITY) {
            ret = xTaskCreate(item->taskFunc, item->name,
                              item->stackDepth, item->param,
                              item->priority, handle_ptr);
        } else {
            ret = xTaskCreatePinnedToCore(item->taskFunc, item->name,
                                          item->stackDepth, item->param,
                                          item->priority, handle_ptr,
                                          item->core_id);
        }

        if (ret != pdPASS) {
            // 根据项目需要处理错误：可以记录日志、停止创建、或继续
            ESP_LOGE("Task create", "Failed to create task [%s]", item->name);
        } else {
            ESP_LOGI("Task create", "[%s] create OK", item->name);
            created++;
        }
    }
    return created;
}

/**
 * 开始任务 - 依次创建其他任务，最后删除自己
 */
static void StartTask(void *pvParameters)
{
    /* 根据系统需求往任务注册表注册任务信息 */
#if SensorUse /* 使用传感器 */

#endif 
    /* 进入临界区（保护初始化过程不被打断） */
    taskENTER_CRITICAL();
    
    /* 批量创建用户任务 */
    uint16_t created = createTasks_fromTable(taskTable);
    printf("Created %d tasks (total %d registered)", created, task_getItemSize());
    
    taskEXIT_CRITICAL();

    /* 所有任务创建完毕，删除“开始任务”自身, 等价于 vTaskDelete(xStartTaskHandle) */
    vTaskDelete(NULL);  
}

