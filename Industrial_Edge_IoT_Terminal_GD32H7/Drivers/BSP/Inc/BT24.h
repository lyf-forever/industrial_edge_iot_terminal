// #ifndef BT24_H_
// #define BT24_H_

// #include "gd32h7xx.h"
// #include <stdint.h>

// // 接收缓冲区大小
// #define BT_TX_BUFF_SIZE  50
// #define BT_RX_BUFF_SIZE  256
// #define BT_RX_DMA_APPBUFF_SIZE    256
// // 定义串口和具体引脚
// #define BT_PERIPH  UART3
// #define BT_PERIPH_IRQn UART3_IRQn                       // 中断对象
// #define BT_TX_DMA_IRQn DMA0_Channel0_IRQn               // DMA通道中断对象
// #define BT_RX_DMA_IRQn DMA0_Channel1_IRQn               // DMA通道中断对象
// #define RCU_BT_PERIPH RCU_UART3                         // 串口时钟
// #define BT_TX_PORT GPIOA                                // 串口Pin TX GPIO
// #define BT_RX_PORT GPIOA                                // 串口Pin RX GPIO
// #define BT_TX_PIN GPIO_PIN_0                            // 串口Pin TX Pin
// #define BT_RX_PIN GPIO_PIN_1                            // 串口Pin RX Pin
// #define BT_TX_AF  GPIO_AF_8                             // 串口TX复用
// #define BT_RX_AF  GPIO_AF_8                             // 串口TX复用
// #define RCU_BT_TX_GPIO RCU_GPIOA                        // 串口Pin TX GPIO 时钟
// #define RCU_BT_RX_GPIO RCU_GPIOA                        // 串口Pin RX GPIO 时钟
// #define BT_TX_DMA_CH  DMA_CH0                           // 串口TX DMA 通道
// #define BT_RX_DMA_CH  DMA_CH1                           // 串口RX DMA 通道
// #define BT_TX_DMA_REQUEST  DMA_REQUEST_UART3_TX         // 串口TX DMA通道请求标识
// #define BT_RX_DMA_REQUEST  DMA_REQUEST_UART3_RX         // 串口RX DMA通道请求标识

// #define BT_TX_DMA_PERIPH   DMA0                         // 串口TX DMA
// #define BT_RX_DMA_PERIPH   DMA0                         // 串口RX DMA

// #define RCU_BT_TX_DMA_PERIPH   RCU_DMA0                 // 串口TX DMA
// #define RCU_BT_RX_DMA_PERIPH   RCU_DMA0                 // 串口RX DMA

// #define BTX_RDATA_ADDRESS      ((uint32_t)((BT_PERIPH) + 0x00000024U))   /* BTX 数据接收寄存器地址 */
// #define BTX_TDATA_ADDRESS      ((uint32_t)((BT_PERIPH) + 0x00000028U))   /* BTX 数据发送寄存器地址 */

// // 外部变量声明，供 main.c 访问
// extern uint8_t bt_dma_rxbuf1[BT_RX_BUFF_SIZE];
// extern uint8_t bt_txbuf[BT_TX_BUFF_SIZE];
// extern uint8_t bt_rx_dma_appBuf[BT_RX_DMA_APPBUFF_SIZE];
// extern volatile uint16_t bt_rxbuf1_newlen;        
// extern volatile uint16_t bt_rxbuf2_newlen;        

// extern volatile uint16_t bt_dma_head;                   /* DMA当前写入位置（中断内更新）*/
// extern volatile uint16_t bt_app_tail1;                  /* 应用程序已处理位置（主循环更新）*/
// extern volatile uint16_t bt_app_tail2;                  /* 应用程序已处理位置（主循环更新）*/
// extern volatile uint8_t  bt_dma_need_switch;            /* 空闲帧接收完成标志（可选）*/

// #define BT_IRQHandler UART3_IRQHandler    // 中断处理函数
// #define BT_TX_DMA_Channel_IRQHandler   DMA0_Channel0_IRQHandler
// #define BT_RX_DMA_Channel_IRQHandler   DMA0_Channel1_IRQHandler

// /* 模块状态 */
// typedef enum {
//     BT_STATE_UNINIT = 0,
//     BT_STATE_IDLE,              // 空闲，未连接
//     BT_STATE_CMD_MODE,          // AT命令模式
//     BT_STATE_TRANS_MODE,        // 透传模式
//     BT_STATE_CONNECTED          // 已连接
// } bt24_state_t;

// /* DMA缓冲区标志 */
// typedef enum{
//     DMA_RXBUF1 = 1,
//     DMA_RXBUF2
// } dma_rxbuf_t;

// #define BT24_CMD_TIMEOUT        1000    // AT 指令响应超时时间（ms）

// // 函数声明
// void BT24_init(uint32_t baudrate);
// uint8_t BT24_SendATCmd(uint8_t *cmd, uint8_t *expected_response, uint32_t timeout_ms);
// void BT24_SetName(uint8_t *name);
// void BT24_SetBaudrate(uint32_t baud);
// void BT24_SetMasterMode(void);
// void BT24_SetSlaveMode(void);
// void BT24_ConnectToDevice(uint8_t *mac);
// void BT24_GetVersion(void);
// void BT24_Reset(void);
// void BT24_EnterCmdMode(void);
// void BT24_EnterTransMode(void);
// bt24_state_t BT24_GetState(void);

// int  bt_send_dma(uint8_t *data, uint16_t len);
// void bt_send_byte(uint32_t bt_periph,uint8_t ch);
// void bt_send_buffer(uint32_t bt_periph,uint8_t *data, uint16_t len);

// void process_data(uint8_t* data, uint16_t len);
// uint16_t BT24_UpdateAppBuffer(void);

// #endif /* BT24_H_ */


#include "gd32h7xx.h"
#include <stdint.h>

// 定义串口和具体引脚
#define BT_PERIPH  UART3
#define BT_PERIPH_IRQn UART3_IRQn                       // 中断对象
#define BT_TX_DMA_IRQn DMA0_Channel0_IRQn               // DMA通道中断对象
#define BT_RX_DMA_IRQn DMA0_Channel1_IRQn               // DMA通道中断对象
#define RCU_BT_PERIPH RCU_UART3                         // 串口时钟
#define BT_TX_PORT GPIOA                                // 串口Pin TX GPIO
#define BT_RX_PORT GPIOA                                // 串口Pin RX GPIO
#define BT_TX_PIN GPIO_PIN_0                            // 串口Pin TX Pin
#define BT_RX_PIN GPIO_PIN_1                            // 串口Pin RX Pin
#define BT_TX_AF  GPIO_AF_8                             // 串口TX复用
#define BT_RX_AF  GPIO_AF_8                             // 串口TX复用
#define RCU_BT_TX_GPIO RCU_GPIOA                        // 串口Pin TX GPIO 时钟
#define RCU_BT_RX_GPIO RCU_GPIOA                        // 串口Pin RX GPIO 时钟
#define BT_TX_DMA_CH  DMA_CH0                           // 串口TX DMA 通道
#define BT_RX_DMA_CH  DMA_CH1                           // 串口RX DMA 通道
#define BT_TX_DMA_REQUEST  DMA_REQUEST_UART3_TX         // 串口TX DMA通道请求标识
#define BT_RX_DMA_REQUEST  DMA_REQUEST_UART3_RX         // 串口RX DMA通道请求标识

#define BT_TX_DMA_PERIPH   DMA0                         // 串口TX DMA
#define BT_RX_DMA_PERIPH   DMA0                         // 串口RX DMA

#define RCU_BT_TX_DMA_PERIPH   RCU_DMA0                 // 串口TX DMA
#define RCU_BT_RX_DMA_PERIPH   RCU_DMA0                 // 串口RX DMA

#define BTX_RDATA_ADDRESS      ((uint32_t)((BT_PERIPH) + 0x00000024U))   /* BTX 数据接收寄存器地址 */
#define BTX_TDATA_ADDRESS      ((uint32_t)((BT_PERIPH) + 0x00000028U))   /* BTX 数据发送寄存器地址 */

#define DMA_RX_BUF_SIZE    256
#define DMA_TX_BUF_SIZE    256

#define BT_IRQHandler UART3_IRQHandler    // 中断处理函数
#define BT_TX_DMA_Channel_IRQHandler   DMA0_Channel0_IRQHandler
//#define BT_RX_DMA_Channel_IRQHandler   DMA0_Channel1_IRQHandler

extern uint8_t dma_tx_buf[DMA_TX_BUF_SIZE];
extern uint8_t dma_rx_buf0[DMA_RX_BUF_SIZE];
extern uint8_t dma_rx_buf1[DMA_RX_BUF_SIZE];
extern volatile uint8_t  dma_active_buf;       // 0: buf0 正在被 DMA 使用; 1: buf1
extern volatile uint8_t  dma_full_buf_idx;     // 哪个缓冲区已满 (0 或 1)
extern volatile uint16_t dma_full_len;         // 该缓冲区的有效数据长度
extern volatile uint8_t  dma_frame_ready;      // 等于1表示有一帧待处理
extern volatile uint8_t  bt_tx_busy;           // 发送忙标志

void bt24_init(uint32_t baudrate);
int bt_send_dma(uint8_t *data, uint16_t len);