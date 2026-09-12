/*****************************************************************************
 * HC05.c
 *  HC‑05 蓝牙模块 AT 指令控制实现（基于串口 DMA）
 *****************************************************************************/

#include "HC05.h"
#include <stdbool.h>
#include "bsp_8080_lcd.h"
#include "lcd_font.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "gd32h7xx_it.h"

// 全局变量定义
uint8_t hc_txbuf[HC_TX_BUFF_SIZE];
uint8_t hc_rxbuf[HC_RX_BUFF_SIZE];
uint8_t hc_rx_dma_appBuf[HC_RX_DMA_APPBUFF_SIZE];

volatile uint16_t hc_dma_head = 0;                  /* DMA当前写入位置（中断中更新）*/
volatile uint16_t hc_app_tail = 0;                  /* 应用程序已处理位置（主循环更新）*/
volatile uint8_t  hc_rx_frame_complete = 0;         /* 空闲帧接收完成标志（可选）*/

volatile uint16_t hc_newLen = 0;        /* hc串口UART4的接收新帧长度 */

// 用于解析数据的临时缓冲区
static uint8_t parse_buffer[HC_RX_BUFF_SIZE];
static volatile bool hcTx_busy = false;    // 发送忙标志

/**
 * @brief  GPIO 配置 PA15(TX), PA8(RX)
 */
static void hc_gpio_config(void)
{
    // 使能GPIO时钟
    rcu_periph_clock_enable(RCU_HC_TX_GPIO);
    rcu_periph_clock_enable(RCU_HC_RX_GPIO);
    
    // PA15 复用推挽输出 (TX)
    gpio_af_set(HC_TX_GPIO_PORT,HC_TX_GPIO_AF,HC_TX_PIN);
    gpio_mode_set(HC_TX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, HC_TX_PIN);
    gpio_output_options_set(HC_TX_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, HC_TX_PIN);
    // PA8 浮空输入 (RX)
    gpio_af_set(HC_RX_GPIO_PORT,HC_RX_GPIO_AF,HC_RX_PIN);
    gpio_mode_set(HC_RX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, HC_RX_PIN);
    gpio_output_options_set(HC_RX_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, HC_RX_PIN);
}

/**
 * @brief  DMA 配置
 */
static void hc_dma_config(void)
{
    // 使能DMA和DMAMUX时钟
    rcu_periph_clock_enable(RCU_HC_TX_DMA_PERIPH);
    rcu_periph_clock_enable(RCU_HC_RX_DMA_PERIPH);
    rcu_periph_clock_enable(RCU_DMAMUX);

    dma_single_data_parameter_struct dma_init_struct;

    dma_deinit(HC_TX_DMA_PERIPH, HC_TX_DMA_CH);
    dma_deinit(HC_RX_DMA_PERIPH, HC_RX_DMA_CH);

    dma_single_data_para_struct_init(&dma_init_struct);
    // --- 配置 DMA0 Channel 2 (用于 HC_PERIPH TX) ---
    dma_init_struct.request = HC_TX_DMA_REQUEST;
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr = (uint32_t)hc_txbuf;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    dma_init_struct.number = 0; // 初始为0，发送时再设置
    dma_init_struct.periph_addr = (uint32_t)&USART_TDATA(UART4);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_MEDIUM;
    dma_single_data_mode_init(HC_TX_DMA_PERIPH, HC_TX_DMA_CH, &dma_init_struct);
    
    /* 使能发送完成中断 */
    dma_interrupt_enable(HC_TX_DMA_PERIPH, HC_TX_DMA_CH, DMA_INT_FTF);
    nvic_irq_enable(HC_TX_DMA_IRQn, 4, 0);

    // --- 配置 DMA0 Channel 3 (用于 HC_PERIPH RX) ---
    dma_init_struct.request = HC_RX_DMA_REQUEST;
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr = (uint32_t)hc_rxbuf;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number = HC_RX_BUFF_SIZE; // 缓冲区大小
    dma_init_struct.periph_addr = (uint32_t)&USART_RDATA(UART4);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_ENABLE; // 关闭循环模式
    dma_single_data_mode_init(HC_RX_DMA_PERIPH, HC_RX_DMA_CH, &dma_init_struct);
    /* 使能接收完成中断 */
    dma_interrupt_enable(HC_RX_DMA_PERIPH, HC_RX_DMA_CH, DMA_INT_HTF);
    dma_interrupt_enable(HC_RX_DMA_PERIPH, HC_RX_DMA_CH, DMA_INT_FTF);
    nvic_irq_enable(HC_RX_DMA_IRQn, 4, 0);
}


/**
 * @brief 初始化 HC‑05 通信（需确保模块已进入 AT 模式）(包含DMA和GPIO) 
 */
void hc05_init(uint32_t baudrate)
{
    /* 1. 开启时钟 */
    rcu_periph_clock_enable(RCU_HC_PERIPH);
    /* 2. 配置DMA */
    hc_dma_config();
    /* 3. 配置GPIO */
    hc_gpio_config();
    /* 4. 配置HC_PERIPH */
    usart_deinit(HC_PERIPH);
    usart_baudrate_set(HC_PERIPH, baudrate);
    usart_word_length_set(HC_PERIPH, USART_WL_8BIT);
    usart_stop_bit_set(HC_PERIPH, USART_STB_1BIT);
    usart_parity_config(HC_PERIPH, USART_PM_NONE);
    usart_hardware_flow_rts_config(HC_PERIPH, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(HC_PERIPH, USART_CTS_DISABLE);
    // 开启接收和发送
    usart_receive_config(HC_PERIPH, USART_RECEIVE_ENABLE);
    usart_transmit_config(HC_PERIPH, USART_TRANSMIT_ENABLE);
    // 使能DMA接收和发送
    usart_dma_receive_config(HC_PERIPH, USART_RECEIVE_DMA_ENABLE);
    usart_dma_transmit_config(HC_PERIPH, USART_TRANSMIT_DMA_ENABLE);
    // 开启空闲中断 (IDLE Interrupt)
    usart_interrupt_enable(HC_PERIPH, USART_INT_IDLE);
    /* 5. 配置NVIC (中断优先级) */
    nvic_irq_enable(HC_PERIPH_IRQn, 3, 0);
    /* 6. 使能HC_PERIPH */
    usart_enable(HC_PERIPH);
    /* 7. 使能DMA通道 先开启接收DMA，让它时刻准备接收 */
    dma_channel_enable(HC_RX_DMA_PERIPH, HC_RX_DMA_CH);
    /* 8. 简单延时等待模块稳定 */
    delay_ms(30);
}

/* 阻塞发送单字节 */
void hc_send_byte(uint32_t hc_periph, uint8_t ch)
{
    usart_data_transmit(hc_periph, ch);
    while (usart_flag_get(hc_periph, USART_FLAG_TBE) == RESET);
}

/* 阻塞发送多字节（非DMA）*/
void hc_send_buffer(uint32_t hc_periph, uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        hc_send_byte(hc_periph, data[i]);
    }
}

/**
 * @brief  通过DMA发送数据（非阻塞）
 */
int hc_send_dma(uint32_t hc_periph, uint32_t dma_periph, dma_channel_enum channelx, uint8_t *data, uint16_t len)
{
    if (hcTx_busy || len == 0 || len > sizeof(hc_txbuf)) {
        return -1;
    }

    /* 等待当前DMA发送完成 */
    dma_channel_disable(dma_periph, channelx);
    while ((DMA_CHCTL(dma_periph, channelx)) & DMA_CHXCTL_CHEN);

    /* 复制数据到发送缓冲区（也可直接使用原地址，这里简单复制）*/
    for (uint16_t i = 0; i < len; i++) {
        hc_txbuf[i] = data[i];
    }

    dma_transfer_number_config(dma_periph, channelx, len);
    dma_memory_address_config(dma_periph, channelx, 0U, (uint32_t)hc_txbuf);
    dma_periph_address_config(dma_periph, channelx, HC_TDATA_ADDRESS);

    hcTx_busy = true;
    dma_channel_enable(dma_periph, channelx);

    return 0;
}

static void DMA_Clear_rxBuffer(void){
    memset(hc_rxbuf, 0, HC_RX_BUFF_SIZE);
}

/* DMA 发送完成中断 */
void HC_TX_DMA_Channel_IRQHandler(void)
{
    if (dma_interrupt_flag_get(HC_TX_DMA_PERIPH, HC_TX_DMA_CH, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(HC_TX_DMA_PERIPH, HC_TX_DMA_CH, DMA_INT_FLAG_FTF);
        hcTx_busy = false;
    }
}

void HC05_UpdateAppBuffer(void)
{
    /* 检测接收帧完成标志 */
    if(hc_rx_frame_complete){
        /* 清除数据帧传输完成标志 */
        hc_rx_frame_complete = 0;
        uint16_t head_snapshot;
        /* 原子操作获取当前head（防止中断中修改） */
        __disable_irq();
        head_snapshot = hc_dma_head;
        __enable_irq();
        /* 计算未处理的数据长度 */
        if(head_snapshot >= hc_app_tail) {
            hc_newLen = head_snapshot - hc_app_tail;
        } else {
            hc_newLen = head_snapshot + (HC_RX_BUFF_SIZE - hc_app_tail);
        }
        if(hc_newLen > 0) {
            /* 将新数据拷贝到应用缓冲区（耗时操作在主循环）*/
            if(head_snapshot >= hc_app_tail) {
                memcpy(hc_rx_dma_appBuf, &hc_rxbuf[hc_app_tail], hc_newLen);
            } else {
                uint16_t first_part = HC_RX_BUFF_SIZE - hc_app_tail;
                memcpy(hc_rx_dma_appBuf, &hc_rxbuf[hc_app_tail], first_part);
                memcpy(hc_rx_dma_appBuf + first_part, hc_rxbuf, head_snapshot);
            }
            hc_app_tail = head_snapshot;  /* 更新已处理指针 */
        }
    }
}

/* DMA接收完成中断 */
void HC_RX_DMA_Channel_IRQHandler(void){
    /* 检查半传输完成中断 */
    if(dma_interrupt_flag_get(HC_RX_DMA_PERIPH, HC_RX_DMA_CH, DMA_INT_FLAG_HTF) != RESET){
        /* 清除标志位 */
        dma_interrupt_flag_clear(HC_RX_DMA_PERIPH, HC_RX_DMA_CH, DMA_INT_FLAG_HTF);
        /* 更新head = 当前DMA写指针 */
        hc_dma_head = HC_RX_BUFF_SIZE - dma_transfer_number_get(HC_RX_DMA_PERIPH, HC_RX_DMA_CH);
    }
    /* 检查全传输完成中断 */
    if(dma_interrupt_flag_get(HC_RX_DMA_PERIPH, HC_RX_DMA_CH, DMA_INT_FLAG_FTF) != RESET){
        /* 清除标志位 */
        dma_interrupt_flag_clear(HC_RX_DMA_PERIPH, HC_RX_DMA_CH, DMA_INT_FLAG_FTF);
        /* 更新head = 当前DMA写指针 */
        hc_dma_head = HC_RX_BUFF_SIZE - dma_transfer_number_get(HC_RX_DMA_PERIPH, HC_RX_DMA_CH);
    }
}

/**
  * @brief  HC UART6 中断服务函数
  * @note   用于处理空闲中断(IDLE)，配合DMA实现不定长接收
  */
void HC_IRQHandler(void)
{
    /* 检查是否为空闲中断 (IDLE Flag) */
    if (usart_interrupt_flag_get(HC_PERIPH, USART_INT_FLAG_IDLE) != RESET){
        /* 清除空闲中断标志位 */
        usart_interrupt_flag_clear(HC_PERIPH,  USART_INT_FLAG_IDLE);
        usart_data_receive(HC_PERIPH);   /* 读DR清除标志 */
        /* 更新head指针 */
        hc_dma_head = HC_RX_BUFF_SIZE - dma_transfer_number_get(HC_RX_DMA_PERIPH, HC_RX_DMA_CH);
        /* 设置帧传输完成标志，通知主循环处理 */
        hc_rx_frame_complete = 1;
    }
}




/*-----------------------------------------------------------
 * 发送 AT 指令并等待期望的响应
 * 参数：
 *   cmd               : AT 指令字符串（例如 "AT\r\n"）
 *   expected_response : 期望收到的响应子串（例如 "OK" 或 "+NAME:"）
 *   timeout_ms        : 超时时间（毫秒）
 * 返回值：
 *   0 ：成功收到期望响应
 *   1 ：超时或未收到
 *-----------------------------------------------------------*/
uint8_t HC05_SendATCmd(uint8_t *cmd, uint8_t *expected_response, uint32_t timeout_ms)
{
    // 清空旧数据
    DMA_Clear_rxBuffer();
    // 通过 DMA 发送 AT 指令
    hc_send_dma(HC_PERIPH, HC_TX_DMA_PERIPH, HC_TX_DMA_CH, (uint8_t *)cmd, strlen((char*)cmd));
    delay_us(5975); 
    // 等待响应 (简单的超时轮询，可改为信号量等非阻塞方式)
    uint32_t tick_start = get_tick(); // 假设有一个获取系统tick的函数
    
    while ((get_tick() - tick_start) < timeout_ms) {
        if (hc_newLen > 0) {
            // 确保数据以'\0'结尾，便于字符串处理
            if (hc_newLen >= HC_RX_BUFF_SIZE) hc_newLen = HC_RX_BUFF_SIZE - 1;
            memcpy(parse_buffer, hc_rx_dma_appBuf, hc_newLen);
            parse_buffer[hc_newLen] = '\0';
            
            // 检查是否收到了期望的响应
            if (strstr((char*)parse_buffer, (char*)expected_response) != NULL) {
                return 0; // 成功
            }
        } 
        // 简单延时，避免CPU空转
        for (volatile int i = 0; i < 10; i++);
    }
    return 1; // 超时或未收到预期响应
}

/*-----------------------------------------------------------
 * 设置蓝牙设备名称
 * 例：HC05_SetName("MyHC05");
 *-----------------------------------------------------------*/
void HC05_SetName(uint8_t *name)
{
    bsp_8080_lcd_clear(WHITE);
    uint8_t cmd[32];
    sprintf((char*)cmd, "AT+NAME=%s\r\n", (char*)name);
    if (HC05_SendATCmd(cmd, (uint8_t*)"OK", HC05_CMD_TIMEOUT)){
        bsp_8080_lcd_printf("HC05 set name failed\r\n");
    }else{
        bsp_8080_lcd_printf( "HC05 set name succeed\r\n");
    }  
}

/*-----------------------------------------------------------
 * 设置串口波特率（修改后需重新初始化串口）
 * 参数可参考 HC‑05 手册，例如：
 *   1 —— 1200
 *   2 —— 2400
 *   3 —— 4800
 *   4 —— 9600
 *   5 —— 19200
 *   6 —— 38400
 *   7 —— 57600
 *   8 —— 115200
 *-----------------------------------------------------------*/
void HC05_SetBaudrate(uint32_t baud)
{
    bsp_8080_lcd_clear(WHITE);
    uint8_t cmd[16];
    sprintf((char*)cmd, "AT+UART=%d,0,0\r\n", (int)baud);
    if (HC05_SendATCmd(cmd, (uint8_t*)"OK", HC05_CMD_TIMEOUT))
    {
        bsp_8080_lcd_printf("set baudrate failed\r\n");
    }
    else
    {
        bsp_8080_lcd_printf("set baudrate succeed");
    }
}

/*-----------------------------------------------------------
 * 设置为主模式
 *-----------------------------------------------------------*/
void HC05_SetMasterMode(void)
{
    bsp_8080_lcd_clear(WHITE);
    if (HC05_SendATCmd((uint8_t*)"AT+ROLE=1\r\n", (uint8_t*)"OK", HC05_CMD_TIMEOUT))
    {
        bsp_8080_lcd_printf("set Master failed\r\n");
    }
    else
    {
        bsp_8080_lcd_printf("set Master succeed\r\n");
    }
}

/*-----------------------------------------------------------
 * 设置为从模式（默认）
 *-----------------------------------------------------------*/
void HC05_SetSlaveMode(void)
{
    bsp_8080_lcd_clear(WHITE);
    if (HC05_SendATCmd((uint8_t*)"AT+ROLE=0\r\n", (uint8_t*)"OK", HC05_CMD_TIMEOUT))
    {
        bsp_8080_lcd_printf("set Slave failed\r\n");
    }
    else
    {
        bsp_8080_lcd_printf("set Slave succeed\r\n");
    }
}

/*-----------------------------------------------------------
 * 主模式下，连接指定 MAC 地址的设备
 * 参数 mac 格式如："12,34,56,78,9A,BC" （注意用逗号分隔）
 *-----------------------------------------------------------*/
void HC05_ConnectToDevice(uint8_t *mac)
{
    bsp_8080_lcd_clear(WHITE);
    uint8_t cmd[32];
    sprintf((char*)cmd, "AT+BIND=%s\r\n", (char*)mac);   // 绑定地址
    if (HC05_SendATCmd(cmd, (uint8_t*)"OK", HC05_CMD_TIMEOUT))
    {
        bsp_8080_lcd_printf("Bind the target failed\r\n");
    }
    else
    {    
        bsp_8080_lcd_printf("Bind the target succeed\r\n");
    }
}

/*-----------------------------------------------------------
 * 获取固件版本信息
 *-----------------------------------------------------------*/
void HC05_GetVersion(void)
{
    bsp_8080_lcd_clear(WHITE);
    if (HC05_SendATCmd((uint8_t*)"AT+VERSION?\r\n", (uint8_t*)"+VERSION:", HC05_CMD_TIMEOUT))
    {

        bsp_8080_lcd_printf("Get version failed\r\n");
    }
    else
    {
        bsp_8080_lcd_printf("Get version succeed:\r\n%s", hc_rx_dma_appBuf);
    }
}

// 其他常用 AT 指令可参照上述模式自行添加，例如：
// AT+RESET   —— 软复位
// AT+ORGL    —— 恢复出厂设置
// AT+PSWD?   —— 查询配对密码
// AT+STATE?  —— 查询当前连接状态

