/*****************************************************************************
 * HC05.h
 *  HC‑05 蓝牙模块 AT 指令控制接口（基于串口 DMA）
 *****************************************************************************/

#ifndef HC05_H_
#define HC05_H_

#include "gd32h7xx.h"
#include "public.h"
#include <stdint.h>

/* ==========================HC05========================================= */
// 接收缓冲区大小
#define HC_TX_BUFF_SIZE  50U
#define HC_RX_BUFF_SIZE  512U
#define HC_RX_DMA_APPBUFF_SIZE    512U

// AT 指令响应超时时间（ms）
#define HC05_CMD_TIMEOUT        1000

// 定义串口和具体引脚
#define HC_PERIPH  UART4
#define HC_TX_PIN  GPIO_PIN_6
#define HC_RX_PIN  GPIO_PIN_5

#define HC_IRQHandler      UART4_IRQHandler        // 中断处理函数
#define HC_PERIPH_IRQn     UART4_IRQn              // 中断对象
#define HC_TX_DMA_IRQn     DMA0_Channel2_IRQn      // DMA通道中断对象
#define HC_RX_DMA_IRQn     DMA0_Channel3_IRQn      // DMA通道中断对象
#define RCU_HC_PERIPH      RCU_UART4               // 串口时钟
#define HC_TX_GPIO_PORT    GPIOB                   // 串口Pin TX GPIO
#define HC_RX_GPIO_PORT    GPIOB                   // 串口Pin RX GPIO
#define HC_TX_GPIO_AF      GPIO_AF_14              // 串口TX复用
#define HC_RX_GPIO_AF      GPIO_AF_14              // 串口TX复用
#define RCU_HC_TX_GPIO     RCU_GPIOB               // 串口Pin TX GPIO 时钟
#define RCU_HC_RX_GPIO     RCU_GPIOB               // 串口Pin RX GPIO 时钟
#define HC_TX_DMA_CH       DMA_CH2                 // 串口TX DMA 通道
#define HC_RX_DMA_CH       DMA_CH3                 // 串口RX DMA 通道
#define HC_TX_DMA_REQUEST  DMA_REQUEST_UART4_TX    // 串口TX DMA通道请求标识
#define HC_RX_DMA_REQUEST  DMA_REQUEST_UART4_RX    // 串口RX DMA通道请求标识

#define HC_TX_DMA_PERIPH   DMA0                    // 串口TX DMA
#define HC_RX_DMA_PERIPH   DMA0                    // 串口RX DMA

#define RCU_HC_TX_DMA_PERIPH   RCU_DMA0            // 串口TX DMA
#define RCU_HC_RX_DMA_PERIPH   RCU_DMA0            // 串口RX DMA

#define HC_RDATA_ADDRESS      ((uint32_t)((HC_PERIPH) + 0x00000024U))   /* HC USARTX 数据接收寄存器地址 */
#define HC_TDATA_ADDRESS      ((uint32_t)((HC_PERIPH) + 0x00000028U))   /* HC USARTX 数据发送寄存器地址 */

#define HC_IRQHandler                  UART4_IRQHandler
#define HC_TX_DMA_Channel_IRQHandler   DMA0_Channel2_IRQHandler
#define HC_RX_DMA_Channel_IRQHandler   DMA0_Channel3_IRQHandler

// 外部变量声明，供 main.c 访问
extern uint8_t hc_rxbuf[HC_RX_BUFF_SIZE];
extern uint8_t hc_txbuf[HC_TX_BUFF_SIZE];
extern uint8_t hc_rx_dma_appBuf[HC_RX_DMA_APPBUFF_SIZE];

extern volatile uint16_t hc_dma_head;                  /* DMA当前写入位置（中断中更新）*/
extern volatile uint16_t hc_app_tail;                  /* 应用程序已处理位置（主循环更新）*/
extern volatile uint8_t  hc_rx_frame_complete;         /* 空闲帧接收完成标志（可选）*/
       
/* ==========================HC05========================================= */

// 函数声明
void hc05_init(uint32_t baudrate);
int  hc_send_dma(uint32_t hc_periph, uint32_t dma_periph, dma_channel_enum channelx, uint8_t *data, uint16_t len);
void hc_send_byte(uint32_t hc_periph,uint8_t ch);
void hc_send_buffer(uint32_t hc_periph,uint8_t *data, uint16_t len);

void HC05_UpdateAppBuffer(void);

uint8_t HC05_SendATCmd(uint8_t *cmd, uint8_t *expected_response, uint32_t timeout_ms);
void HC05_SetName(uint8_t *name);
void HC05_SetBaudrate(uint32_t baud);
void HC05_SetMasterMode(void);
void HC05_SetSlaveMode(void);
void HC05_ConnectToDevice(uint8_t *mac);
void HC05_GetVersion(void);


#endif /* __HC05_H */