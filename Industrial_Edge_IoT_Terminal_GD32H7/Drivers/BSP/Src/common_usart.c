#include "common_usart.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "bsp_8080_lcd.h"
#include "lcd_font.h"
#include "public.h"
#include "systick.h"
#include <sys/stat.h>


/* For GCC (arm-none-eabi-gcc) we do not need special pragmas to disable semihosting.
   Instead, we provide our own _write function (and optionally other system calls). */

/* USART0 是否已初始化。main 仅初始化 UART3(BT24)/SPI2 链路，USART0 未初始化时
 * TC 标志永远不置位，原实现会让 printf 死等（log_export_all 每秒触发一次，系统必挂）。
 * 未初始化时丢弃输出，避免阻塞主循环。 */
static volatile uint8_t usart0_ready = 0;

/* Redefine _write function, which is called by printf (and other stdio functions) */
int _write(int file, char *ptr, int len)
{
    int i;
    if (!usart0_ready) {
        return len;   /* USART0 未初始化：丢弃输出，不得阻塞 */
    }
    for (i = 0; i < len; i++) {
        /* Wait for the previous character to be sent */
        while (RESET == usart_flag_get(USART_PERIPH, USART_FLAG_TC));
        /* Send the character */
        usart_data_transmit(USART_PERIPH, (uint8_t)ptr[i]);
    }
    return len;
}

/* Optional but recommended: define other system calls to avoid linker warnings */
void _exit(int status)
{
    (void)status;
    while (1);
}

int _close(int file)
{
    return -1;
}

int _lseek(int file, int ptr, int dir)
{
    return 0;
}

int _read(int file, char *ptr, int len)
{
    return 0;
}

int _fstat(int file, struct stat *st)
{
    return 0;
}

int _isatty(int file)
{
    return 1;
}

// 全局变量定义
uint8_t usart_txbuf[USART_TX_BUFF_SIZE];
uint8_t usart_rxbuf[USART_RX_BUFF_SIZE];
uint8_t usart_rx_dma_appBuf[USART_RX_DMA_APPBUFF_SIZE];        

volatile uint8_t  usart_dma_ftf_times = 0;             /* DMA FIFO传输完成标志（中断中更新）*/
volatile uint16_t usart_dma_head = 0;                  /* DMA当前写入位置（中断中更新）*/
volatile uint16_t usart_app_tail = 0;                  /* 应用程序已处理位置（主循环更新）*/
volatile uint8_t  usart_rx_frame_complete = 0;         /* 空闲帧接收完成标志（可选）*/

volatile uint16_t usart_newLen = 0;        /* usart串口USART0的接收新帧长度 */

static volatile bool usart_tx_busy = false;    // 发送忙标志

static void usart_gpio_config(void){
    /* 使能引脚时钟 */
    rcu_periph_clock_enable(RCU_USART_TX_GPIO);     /* 使能串口TX脚时钟 */
    rcu_periph_clock_enable(RCU_USART_RX_GPIO);     /* 使能串口RX脚时钟 */
    /* 设置USARTx_TX引脚的复用功能选择 */
    gpio_af_set(USART_TX_PORT, USART_TX_AF, USART_TX_PIN);
    /* 设置USARTx_RX引脚的复用功能选择 */
    gpio_af_set(USART_RX_PORT, USART_RX_AF, USART_RX_PIN);
    /* USARTx_TX引脚的模式设置 */
    gpio_mode_set(USART_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART_TX_PIN);
    gpio_output_options_set(USART_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, USART_TX_PIN);
    /* USARTx_RX引脚的模式设置 */
    gpio_mode_set(USART_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART_RX_PIN);
    gpio_output_options_set(USART_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, USART_RX_PIN);
}

static void usart_dma_config(void)
{
    // 使能DMA和DMAMUX时钟
    rcu_periph_clock_enable(RCU_USART_TX_DMA_PERIPH);
    rcu_periph_clock_enable(RCU_USART_RX_DMA_PERIPH);
    rcu_periph_clock_enable(RCU_DMAMUX);

    dma_single_data_parameter_struct dma_init_struct;

    dma_deinit(USART_TX_DMA_PERIPH, USART_TX_DMA_CH);
    dma_deinit(USART_RX_DMA_PERIPH, USART_RX_DMA_CH);

    dma_single_data_para_struct_init(&dma_init_struct);
    // --- 配置 DMA0 Channel 4 (用于 USART_PERIPH TX) ---
    dma_init_struct.request = USART_TX_DMA_REQUEST;
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr = (uint32_t)usart_txbuf;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    dma_init_struct.number = 0; // 初始为0，发送时再设置
    dma_init_struct.periph_addr = (uint32_t)&USART_TDATA(USART0);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_MEDIUM;
    dma_single_data_mode_init(USART_TX_DMA_PERIPH, USART_TX_DMA_CH, &dma_init_struct);
    
    /* 使能发送完成中断 */
    dma_interrupt_enable(USART_TX_DMA_PERIPH, USART_TX_DMA_CH, DMA_INT_FTF);
    nvic_irq_enable(USART_TX_DMA_IRQn, 7, 0);

    // --- 配置 DMA0 Channel 5 (用于 USART_PERIPH RX) ---
    dma_init_struct.request = USART_RX_DMA_REQUEST;
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr = (uint32_t)usart_rxbuf;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number = USART_RX_BUFF_SIZE; // 缓冲区大小
    dma_init_struct.periph_addr = (uint32_t)&USART_RDATA(USART0);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_ENABLE; // 开启循环模式
    dma_single_data_mode_init(USART_RX_DMA_PERIPH, USART_RX_DMA_CH, &dma_init_struct); 

    /* 开启DMA接收全传输完成中断 */
    dma_interrupt_enable(USART_RX_DMA_PERIPH, USART_RX_DMA_CH, DMA_INT_FTF);
    nvic_irq_enable(USART_RX_DMA_IRQn, 6, 0);
}


/**
 * @brief       串口X初始化函数
 * @param       bound: 波特率, 根据自己需要设置波特率值
 * @retval      无
 */
void usart_init(uint32_t baudrate)
{
    /* 1. 开启时钟 */
    rcu_periph_clock_enable(RCU_USART_PERIPH);
    /* 2. 配置DMA */
    usart_dma_config();
    /* 3. 配置GPIO */
    usart_gpio_config();
    /* 4. 配置USART_PERIPH */
    usart_deinit(USART_PERIPH);
    usart_baudrate_set(USART_PERIPH, baudrate);
    usart_word_length_set(USART_PERIPH, USART_WL_8BIT);
    usart_stop_bit_set(USART_PERIPH, USART_STB_1BIT);
    usart_parity_config(USART_PERIPH, USART_PM_NONE);
    usart_hardware_flow_rts_config(USART_PERIPH, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART_PERIPH, USART_CTS_DISABLE);
    // 开启接收和发送
    usart_receive_config(USART_PERIPH, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART_PERIPH, USART_TRANSMIT_ENABLE);
    // 使能DMA接收和发送
    usart_dma_receive_config(USART_PERIPH, USART_RECEIVE_DMA_ENABLE);
    usart_dma_transmit_config(USART_PERIPH, USART_TRANSMIT_DMA_ENABLE);
    /* 5. 配置NVIC (中断优先级) */
    nvic_irq_enable(USART_PERIPH_IRQn, 7, 0);
    /* 6. 使能USART_PERIPH */
    usart_enable(USART_PERIPH);
    /* 系统上电时可能会误触发空闲中断，我们需要手动清除一次空闲中断标志位 */
    usart_flag_clear(USART_PERIPH, USART_FLAG_IDLE);
    /* 开启空闲中断 (IDLE Interrupt) */
    usart_interrupt_enable(USART_PERIPH, USART_INT_IDLE);
    /* 7. 使能DMA通道 先开启接收DMA，让它时刻准备接收 */
    dma_channel_enable(USART_RX_DMA_PERIPH, USART_RX_DMA_CH);
    usart0_ready = 1;   /* 允许 printf 输出 */
}

/**
 * @brief  更新应用缓冲区（将新数据从DMA环形缓冲区复制到线性应用缓冲区）
 * @return 实际复制到应用缓冲区的数据长度
 */
void usart_updateAppBuffer(void)
{
    /* 检测接收帧完成标志 */
    if(usart_rx_frame_complete) {
        /* 清除数据帧传输完成标志 */
        usart_rx_frame_complete = 0;
        uint16_t head_snapshot, firstTransleft, transfullbuf_times;
        /* 原子操作获取当前head（防止中断中修改） */
        __disable_irq();
        head_snapshot = usart_dma_head;
        firstTransleft = USART_RX_BUFF_SIZE - usart_app_tail;
        __enable_irq();
        /* 计算未处理的数据长度 */
        if(usart_dma_ftf_times > 0) { 
            usart_newLen = USART_RX_BUFF_SIZE - usart_app_tail + (usart_dma_ftf_times - 1) * USART_RX_BUFF_SIZE + head_snapshot;
            transfullbuf_times = usart_dma_ftf_times - 1;
        }else {
            usart_newLen = head_snapshot - usart_app_tail;
            transfullbuf_times = 0;
        }
        
        if(usart_newLen > 0) {
            if(usart_newLen == head_snapshot - usart_app_tail) {
                memcpy(usart_rx_dma_appBuf, &usart_rxbuf[usart_app_tail], usart_newLen);
            } else {
                if(transfullbuf_times > 0) {
                    bsp_8080_lcd_show_string(10, 20, 200, 20, FONT_ASCII_12_6, \
                         "Received data shows too much", RED);
                    usart_newLen = 0;
                    return;
                }
                memcpy(usart_rx_dma_appBuf, &usart_rxbuf[usart_app_tail], firstTransleft);
                memcpy(usart_rx_dma_appBuf + firstTransleft, usart_rxbuf, head_snapshot);
            }
            usart_app_tail = head_snapshot;  /* 更新已处理指针 */
        } else  return;
        usart_dma_ftf_times = 0;
    }
}

/**
 * @brief  通过DMA发送数据（非阻塞）
 */
int usart0_send_dma(uint8_t *data, uint16_t len)
{
    if (usart_tx_busy || len == 0 || len > sizeof(usart_txbuf)) {
        return -1;
    }

    /* 等待当前DMA发送完成 */
    dma_channel_disable(USART_TX_DMA_PERIPH, USART_TX_DMA_CH);
    while ((DMA_CHCTL(USART_TX_DMA_PERIPH, USART_TX_DMA_CH)) & DMA_CHXCTL_CHEN);

    /* 复制数据到发送缓冲区（也可直接使用原地址，这里简单复制）*/
    for (uint16_t i = 0; i < len; i++) {
        usart_txbuf[i] = data[i];
    }

    dma_transfer_number_config(USART_TX_DMA_PERIPH, USART_TX_DMA_CH, len);
    dma_memory_address_config(USART_TX_DMA_PERIPH, USART_TX_DMA_CH, 0U, (uint32_t)usart_txbuf);
    dma_periph_address_config(USART_TX_DMA_PERIPH, USART_TX_DMA_CH, USARTX_TDATA_ADDRESS);

    usart_tx_busy = true;
    dma_channel_enable(USART_TX_DMA_PERIPH, USART_TX_DMA_CH);

    return 0;
}

// void process_data(uint8_t* data, uint16_t len){
//     usart0_send_dma(data, len);
// }


/* DMA 发送完成中断 */
void USART_TX_DMA_Channel_IRQHandler(void)
{
    if (dma_interrupt_flag_get(USART_TX_DMA_PERIPH, USART_TX_DMA_CH, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(USART_TX_DMA_PERIPH, USART_TX_DMA_CH, DMA_INT_FLAG_FTF);
        usart_tx_busy = false;
    }
}

/* DMA接收完成中断 */
void USART_RX_DMA_Channel_IRQHandler(void){
    /* 检查全传输完成中断 */
    if(dma_interrupt_flag_get(USART_RX_DMA_PERIPH, USART_RX_DMA_CH, DMA_INT_FLAG_FTF) != RESET){
        /* 清除标志位 */
        dma_interrupt_flag_clear(USART_RX_DMA_PERIPH, USART_RX_DMA_CH, DMA_INT_FLAG_FTF);
        usart_dma_ftf_times ++;  // 全传输完成次数 + 1
    }
}

/* 串口空闲中断 - USART0 */
void USART_IRQHandler(void)
{
    // 检查是否为空闲中断 (IDLE Flag)
    if (usart_interrupt_flag_get(USART_PERIPH, USART_INT_FLAG_IDLE) != RESET){
        // 清除空闲中断标志位
        usart_interrupt_flag_clear(USART_PERIPH,  USART_INT_FLAG_IDLE);
        usart_data_receive(USART_PERIPH);   // 读DR清除标志
        // 更新head指针
        usart_dma_head = USART_RX_BUFF_SIZE - dma_transfer_number_get(USART_RX_DMA_PERIPH, USART_RX_DMA_CH);
        // 设置帧传输完成标志，通知主循环处理
        usart_rx_frame_complete = 1;
    }
}

