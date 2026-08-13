#ifndef __APP_TASKS_H_
#define __APP_TASKS_H_

/**
 * @file app_tasks.h
 * @brief 应用任务注册表
 *
 * 沿用原项目的 taskItem / TASK_ITEM 表驱动任务注册机制，
 * 修复了宏字段名与结构体成员不一致的 bug，并扩展支持各服务任务。
 */

#include "sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ===================== 任务注册信息结构体 ===================== */
/* 任务函数指针类型 */
typedef TaskFunction_t app_task_func_t;

typedef struct {
    app_task_func_t  taskFunc;      /* 任务函数 */
    const char      *name;          /* 任务名称（必须为字符串常量） */
    uint32_t         stackDepth;    /* 栈深度（字节） */
    void            *param;         /* 任务参数 */
    UBaseType_t      priority;      /* 任务优先级 */
    BaseType_t       coreID;        /* 运行核心：0/1，或 tskNO_AFFINITY */
    TaskHandle_t    *handle;        /* 句柄指针（可为 NULL） */
} taskItem;

/* 提供宏简化注册（字段名与结构体成员严格一致） */
#define TASK_ITEM(_func, _name, _stack, _param, _prio, _core, _handle_ptr) \
    { .taskFunc = (_func), .name = (_name), .stackDepth = (_stack), \
      .param = (_param), .priority = (_prio), .coreID = (_core), \
      .handle = (_handle_ptr) }

/* ===================== 任务栈深宏定义 ===================== */
#define TASK_START_STK_SIZE     2048    /* 开始任务栈深 */
#if SensorUse
#define TASK_SENSOR_STK_SIZE    3072    /* 传感器任务栈深 */
#endif
#if LinkUse
#define TASK_LINK_RX_STK_SIZE   3072    /* 链路接收任务栈深 */
#endif
#if CloudUse
#define TASK_CLOUD_STK_SIZE     4096    /* 云端桥接任务栈深 */
#endif
#if Ws2812Use
#define TASK_WS2812_STK_SIZE    2048    /* WS2812 任务栈深(预留，现由HSM驱动) */
#endif
#if EventBusUse && LinkUse
#define TASK_EVENT_IPC_STK_SIZE 3072    /* 跨核事件桥接任务栈深 */
#endif
#if AiUse
#define TASK_AI_STK_SIZE        4096    /* 边缘 AI 推理任务栈深 */
#endif

/* ===================== 任务优先级宏定义 ===================== */
#define TASK_START_PRIO         3       /* 开始任务优先级 */
#if SensorUse
#define TASK_SENSOR_PRIO        4       /* 传感器任务优先级 */
#endif
#if LinkUse
#define TASK_LINK_RX_PRIO       6       /* 链路接收任务优先级(较高，保证实时) */
#endif
#if CloudUse
#define TASK_CLOUD_PRIO         2       /* 云端桥接任务优先级(较低，后台) */
#endif
#if Ws2812Use
#define TASK_WS2812_PRIO        1       /* WS2812 状态指示任务优先级(最低) */
#endif
#if EventBusUse && LinkUse
#define TASK_EVENT_IPC_PRIO     5       /* 跨核事件桥接任务优先级 */
#endif
#if AiUse
#define TASK_AI_PRIO            3       /* 边缘 AI 推理任务优先级 */
#endif

/* ===================== API 声明 ===================== */

/* 系统级软件初始化（在 init 表之后、任务创建之前调用） */
void System_Init(void);

/* 根据任务注册表批量创建任务（无参版本，供 main 调用） */
void App_Tasks_Create(void);

/* 开始任务（注册并创建其余 worker 任务后删除自身） */
void StartTask(void *pvParameters);

/* 传感器任务（本地 MQ2 采样） */
void SensorTask(void *pvParameters);

/* 边缘 AI 推理任务（架构 3.13 状态巡检） */
void AiTask(void *pvParameters);

#endif /* __APP_TASKS_H_ */