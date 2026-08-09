#ifndef __APP_TASKS_H_
#define __APP_TASKS_H_

#include "sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 任务注册信息结构体 */
typedef struct {
    TaskFunction_t   taskFunc;      // 任务函数
    const char      *name;          // 任务名称（必须为字符串常量）
    uint32_t         stackDepth;    // 栈深度（字节）
    void            *param;         // 任务参数
    UBaseType_t      priority;      // 任务优先级
    BaseType_t       coreID;        // 运行核心：0/1，或 tskNO_AFFINITY
    TaskHandle_t    *handle;        // 句柄指针（可为 NULL）
} taskItem;

/* 提供宏简化注册 */
#define TASK_ITEM(_func, _name, _stack, _param, _prio, _core, _handle_ptr) \
    { .task_func = (_func), .name = (_name), .stack_depth = (_stack), \
      .param = (_param), .priority = (_prio), .core_id = (_core), \
      .handle = (_handle_ptr) }

/* 任务栈深宏定义 */
#define TASK_START_STK_SIZE    256     // 开始任务栈深
#if SensorUse  
#define TASK_SENSOR_STK_SIZE   1024    // 传感器任务栈深
#endif

/* 任务优先级宏定义 */
#define TASK_START_PRIO        1       // 开始任务优先级
#if SensorUse
#define TASK_SENSOR_PRIO       2       // 传感器任务优先级   
#endif

/* API declare */
void System_Init(void);
uint16_t createTasks_fromTable(const taskItem *table);

#endif 

