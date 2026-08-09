#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sys.h"
#include "mq2.h"
#include "app_tasks.h"

/* 初始化函数指针类型：返回成功/失败，可传入用户参数 */
typedef void (*initInterface)(void *arg);

/* 初始化阶段枚举（可根据需要扩展） */
typedef enum {
    INIT_STAGE_HARDWARE = 0,   // 硬件初始化阶段（先执行）
    INIT_STAGE_SOFTWARE,       // 软件初始化阶段（后执行）
    INIT_STAGE_MAX
} initStage;

/* 初始化表项：统一的描述结构体 */
typedef struct {
    const char     *name;        // 步骤名称，用于日志或调试
    initStage       stage;       // 所属初始化阶段
    initInterface   init_func;   // 初始化函数指针
    void           *arg;         // 传递给初始化函数的参数
    uint32_t        depends_on;  // 依赖标记（位掩码或序号），0 表示无依赖
} initItem;

/* 提供一个宏来简化表项定义 */
#define INIT_ITEM(_name, _stage, _func, _arg, _dep)  \
    { .name = (_name), .stage = (_stage), .init_func = (_func), \
      .arg = (_arg), .depends_on = (_dep) }

/* 静态初始化表（放在 main 内，或用全局数组） */
static const initItem initTable[] = { 
#if SensorUse /* 使用传感器 */
#if Mq2Use  /* 使用MQ2 */
    INIT_ITEM("mq2", INIT_STAGE_HARDWARE, mq2_drv_init, NULL, 0),
#endif
#endif

};

/* 初始化调度器：按阶段顺序执行表内所有项 */
static void initTable_run(const initItem *table, size_t count) {
    for (initStage stage = INIT_STAGE_HARDWARE; stage < INIT_STAGE_MAX; stage++) 
    {
        for (size_t i = 0; i < count; i++) 
        {
            if (table[i].stage == stage) {
                ESP_LOGI("Hardware init", "Running [%s]...", table[i].name);
                if (table[i].init_func) {
                    table[i].init_func(table[i].arg);
                }
                ESP_LOGI("Hardware init", "[%s] init finished", table[i].name);
            }
        }
    }
}

/* 应用配置，包括模块片上外设初始化、软件初始化 */
static inline void System_Module_Init(void)
{
    initTable_run(initTable, sizeof(initTable) / sizeof(initTable[0]));
}

/* 应用测试 */
static void App_Test(void)
{

}

void app_main(void)
{
    /* 应用配置，包括模块片上外设初始化、软件初始化 */
    System_Module_Init();
	
	/* 系统初始化 */
	System_Init();
	
    /* 应用测试 */
    App_Test();

    /* 创建开始任务 */
    createTasks_fromTable();

    /* 正常情况下不会执行到这里 */
    while(1)
    {
        
    }
}

