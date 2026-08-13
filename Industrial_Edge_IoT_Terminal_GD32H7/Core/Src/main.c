#include "main.h"
#include "gd32h7xx.h"
#include "gd32h7xx_gpio.h"
#include "gd32h7xx_it.h"
#include "lcd_font.h"
#include "led.h"
#include "systick.h"
#include "BT24.h"
#include "common_usart.h"
#include "link_protocol.h"
#include "sdram.h"
#include "MPU.h"
#include "public.h"
#include "bsp_8080_lcd.h"
#include "malloc.h"
#include "log_ringbuf.h"
#include "event_bus.h"
#include "event_ipc.h"
#include "spi_slave_link.h"
#include <stdint.h>

link_parse_ctx_t bt_link_ctx;

/* 日志环形缓冲存储区（64KB，置于 AXI SRAM；若需 SDRAM 可改用 SET_SDRAM 并配链接段） */
static uint8_t log_buf[LOG_BUF_SIZE];

/* 软定时器回调（架构 3.8：由 SysTick 经 driver_tic_inc 驱动） */
static void tick_10ms_cb(void)
{
    /* 10ms 周期：LED3 闪烁指示系统运行。
     * 注意：LED2 位于 PB14，与 SPI2 从机 MISO（spi_slave_link）冲突，
     * 不能再作 GPIO 使用。 */
    gpio_bit_toggle(LED3_PORT, LED3_PIN);
}

static void tick_100ms_cb(void)
{
    /* 100ms 周期：预留（可做链路健康检测等长周期任务） */
}

void dma_switch_and_process(void)
{
    __disable_irq();
    uint8_t *buf = (dma_full_buf_idx == 0) ? dma_rx_buf0 : dma_rx_buf1;
    uint16_t rec_len = dma_full_len;
    /* DMA 已向缓冲区写入新数据：先失效 D-Cache 行，避免 CPU 读到陈旧缓存数据 */
    if (rec_len > 0) {
        dcache_invalidate_region((uint32_t)buf, rec_len);
    }
    /* 跨核事件桥接：字节流喂入 link 解析（含转义解码） */
    event_ipc_feed(buf, rec_len);
    
    // 切换 DMA 到另一个缓冲区
    dma_channel_disable(DMA0, BT_RX_DMA_CH);
    
    if (dma_active_buf == 0) {
        dma_memory_address_config(DMA0, BT_RX_DMA_CH, 0, (uint32_t)dma_rx_buf1);
        dma_active_buf = 1;
    } else {
        dma_memory_address_config(DMA0, BT_RX_DMA_CH, 0, (uint32_t)dma_rx_buf0);
        dma_active_buf = 0;
    }
    dma_transfer_number_config(DMA0, BT_RX_DMA_CH, DMA_RX_BUF_SIZE);
    dma_channel_enable(DMA0, BT_RX_DMA_CH);
    __enable_irq();
}

int main(void)
{
    /* 系统时钟初始化（可根据需要调整）*/
    // 这里使用默认时钟，如需高性能，请配置系统时钟至400MHz等
    SystemInit(); // 请根据你的系统时钟配置函数调用
    /* CPU Cache使能 */
    sys_cache_enable();  // 使能CPU缓存，提高系统运行效率
    /* NVIC中断优先级分组设置 */
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    /* 系统时基初始化 */
    systick_cfg();
    /* 公共驱动初始化 */
    delay_init();   
    /* 保护相关存储区域 */
    mpu_memory_protection();  // 配置内存保护单元，保护关键内存区域不被意外修改
    /* SDRAM 初始化 */
    sdram_init(EXMC_SDRAM_DEVICE0);  // 初始化外部SDRAM存储器，用于扩展系统内存
    /* LCD初始化 */
    bsp_8080_lcd_init();
    bsp_8080_lcd_printf_init(10, 109, bsp_8080_lcd_parameter.width - 1, 369, FONT_ASCII_24_12, WHITE, BLUE);
    bsp_8080_lcd_printf("dual-mcu link demo\r\n");
    /* 初始化LED矩阵 */
    led_gpio_init();

    /* ---------- 架构落地（software_architecture 文档） ---------- */
    /* 架构 3.6：双池内存分配器（内部 SRAM + 外部 SDRAM） */
    my_mem_init(SRAMIN);
    my_mem_init(SRAMEX);
    bsp_8080_lcd_printf("mem: SRAM %u%% SDRAM %u%%\r\n",
        (unsigned)my_mem_perused(SRAMIN), (unsigned)my_mem_perused(SRAMEX));

    /* 架构 3.9：结构化日志环缓冲（SDRAM 64KB） */
    log_ringbuf_init(log_buf, LOG_BUF_SIZE, true);
    log_printf(LOG_LEVEL_INFO, "system boot, log ringbuf ready");

    /* 架构 3.8：软定时器回调注册（1/10/100ms 三轮） */
    driver_tick_handle[0].tick_task_callback = NULL;      /* 1ms 槽（预留） */
    driver_tick_handle[1].tick_task_callback = tick_10ms_cb;   /* 10ms 槽 */
    driver_tick_handle[2].tick_task_callback = tick_100ms_cb;  /* 100ms 槽 */

    /* 架构 2.8/2.9：事件总线 + 跨核事件桥接（挂 BT24 UART3 链路） */
    event_bus_init();
    event_ipc_init();

    /* 架构 3.3b：HS-SPI 从机链路（双链路冗余，SPI2） */
    spi_slave_link_init();

    /* ---------------------------串口------------------------------------ */
    /* BT24初始化 */
    bt24_init(9600);     // BT24出厂波特率为9600
    // /* BT24帧解析结构体初始化 */
    link_parse_init(&bt_link_ctx);
/* ---------------------------串口------------------------------------ */
    uint64_t last_log_export = 0;   /* v2.0：日志导出按 driver_tick 1s 周期门控 */
    while (1) {
        if (dma_frame_ready) {
            dma_frame_ready = 0;
            dma_switch_and_process();
        }
        /* 架构 3.3b：SPI 从机链路轮询（接收主机帧进同一 link 解析管线，
         * v2.0 起非阻塞：不再等待 SPI 传输完成） */
        spi_slave_link_poll();
        delay_ms(20);
        /* 周期导出日志（1s 一次，匹配注释承诺；避免每 20ms 全缓冲扫描
         * 拖慢 UART0 printf） */
        if (driver_tick - last_log_export >= 1000) {
            last_log_export = driver_tick;
            log_export_all();
        }
        /* 其他任务 */
    }
}