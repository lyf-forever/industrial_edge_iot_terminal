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
#include <stdint.h>

link_parse_ctx_t bt_link_ctx;

void dma_switch_and_process(void)
{
    __disable_irq();
    uint8_t *buf = (dma_full_buf_idx == 0) ? dma_rx_buf0 : dma_rx_buf1;
    uint16_t rec_len = dma_full_len;
    if(rec_len == 0) gpio_bit_toggle(LED3_PORT, LED3_PIN);
    link_parse_buffer(&bt_link_ctx, buf, rec_len);
    
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
    bsp_8080_lcd_printf("link protocol demo\r\n");
    /* 初始化LED矩阵 */
    led_gpio_init();
    /* ---------------------------串口------------------------------------ */
    /* BT24初始化 */
    bt24_init(9600);     // BT24出厂波特率为9600
    // /* BT24帧解析结构体初始化 */
    link_parse_init(&bt_link_ctx);
/* ---------------------------串口------------------------------------ */
    while (1) {
        if (dma_frame_ready) {
            dma_frame_ready = 0;
            dma_switch_and_process();
        }
        delay_ms(20);
        // 其他任务
    }
}


