#ifndef COMMON_USART_H_
#define COMMON_USART_H_

#include "stdio.h"
#include "gd32h7xx.h"

// 接收缓冲区大小
#define USART_TX_BUFF_SIZE  50
#define USART_RX_BUFF_SIZE  256
#define USART_RX_DMA_APPBUFF_SIZE    256
// 定义串口和具体引脚
#define USART_PERIPH  USART0
#define USART_PERIPH_IRQn USART0_IRQn                       // 中断对象
#define USART_TX_DMA_IRQn DMA0_Channel4_IRQn               // DMA通道中断对象
#define USART_RX_DMA_IRQn DMA0_Channel5_IRQn               // DMA通道中断对象
#define RCU_USART_PERIPH RCU_USART0                         // 串口时钟
#define USART_TX_PORT GPIOB                                // 串口Pin TX GPIO
#define USART_RX_PORT GPIOB                                // 串口Pin RX GPIO
#define USART_TX_PIN GPIO_PIN_14                            // 串口Pin TX Pin
#define USART_RX_PIN GPIO_PIN_15                            // 串口Pin RX Pin
#define USART_TX_AF  GPIO_AF_4                             // 串口TX复用
#define USART_RX_AF  GPIO_AF_4                             // 串口TX复用
#define RCU_USART_TX_GPIO RCU_GPIOB                        // 串口Pin TX GPIO 时钟
#define RCU_USART_RX_GPIO RCU_GPIOB                        // 串口Pin RX GPIO 时钟
#define USART_TX_DMA_CH  DMA_CH4                           // 串口TX DMA 通道
#define USART_RX_DMA_CH  DMA_CH5                           // 串口RX DMA 通道
#define USART_TX_DMA_REQUEST  DMA_REQUEST_USART0_TX         // 串口TX DMA通道请求标识
#define USART_RX_DMA_REQUEST  DMA_REQUEST_USART0_RX         // 串口RX DMA通道请求标识

#define USART_TX_DMA_PERIPH   DMA0                         // 串口TX DMA
#define USART_RX_DMA_PERIPH   DMA0                         // 串口RX DMA

#define RCU_USART_TX_DMA_PERIPH   RCU_DMA0                 // 串口TX DMA
#define RCU_USART_RX_DMA_PERIPH   RCU_DMA0                 // 串口RX DMA

#define USARTX_RDATA_ADDRESS      ((uint32_t)((USART_PERIPH) + 0x00000024U))   /* USARTX 数据接收寄存器地址 */
#define USARTX_TDATA_ADDRESS      ((uint32_t)((USART_PERIPH) + 0x00000028U))   /* USARTX 数据发送寄存器地址 */

// 外部变量声明，供 main.c 访问
extern uint8_t usart_rxbuf[USART_RX_BUFF_SIZE];
extern uint8_t usart_txbuf[USART_TX_BUFF_SIZE];
extern uint8_t usart_rx_dma_appBuf[USART_RX_DMA_APPBUFF_SIZE];
extern volatile uint16_t usart_newLen;        
extern volatile uint8_t usart_dma_ftf_times;
extern volatile uint16_t usart_dma_head;                  /* DMA当前写入位置（中断内更新）*/
extern volatile uint16_t usart_app_tail;                  /* 应用程序已处理位置（主循环更新）*/
extern volatile uint8_t  usart_rx_frame_complete;         /* 空闲帧接收完成标志（可选）*/

#define USART_IRQHandler USART0_IRQHandler    // 中断处理函数
#define USART_TX_DMA_Channel_IRQHandler   DMA0_Channel4_IRQHandler
#define USART_RX_DMA_Channel_IRQHandler   DMA0_Channel5_IRQHandler

// 函数声明
int  usart0_send_dma(uint8_t *data, uint16_t len);
void process_data(uint8_t* data, uint16_t len);
void usart_updateAppBuffer(void);
void usart_init(uint32_t baudrate);                 /* 串口初始化函数 */

#endif


