// /*****************************************************************************
//  * BT24.c
//  *  DX-BT24 蓝牙透传模块 AT 指令控制接口（基于串口 DMA）
//  *****************************************************************************/
// #include "BT24.h"
// #include "bsp_8080_lcd.h"
// #include "gd32h7xx_dma.h"
// #include "lcd_font.h"
// #include "gd32h7xx_it.h"
// #include <stdint.h>


// /* 本文件内使用UART3 DMA0 CH0 CH1 */

// // 用于解析数据的临时缓冲区
// static uint8_t parse_buffer[DMA_RX_BUF_SIZE];
// static volatile bool bt24_cmd_pending = false;    // AT命令等待响应标志
// static bt24_state_t bt24_state = BT_STATE_UNINIT;

// dma_rxbuf_t dma_rxbuf = DMA_RXBUF1;

// // 全局变量定义
// uint8_t dma_tx_buf[BT_TX_BUFF_SIZE];
// uint8_t bt_dma_rxbuf1[DMA_RX_BUF_SIZE];    // DMA双缓冲区 buf1
// uint8_t bt_dma_rxbuf2[DMA_RX_BUF_SIZE];    // DMA双缓冲区 buf2
// uint8_t bt_rx_dma_appBuf[BT_RX_DMA_APPBUFF_SIZE];        

// volatile uint16_t btPeriph_rxLen = 0;
// volatile uint8_t btPeriph_rxComplete = 0;

// volatile uint16_t bt_dma_head = 0;                  /* DMA当前写入位置（中断中更新）*/
// volatile uint16_t bt_app_tail1 = 0;                 /* 应用程序已处理位置1（主循环更新）*/
// volatile uint16_t bt_app_tail2 = 0;                 /* 应用程序已处理位置2（主循环更新）*/
// volatile uint8_t  bt_dma_need_switch = 0;           /* 空闲帧接收完成标志（可选）*/

// volatile uint16_t bt_rxbuf1_newlen = 0;        /* bt串口UART3的接收新帧长度 */

// static volatile bool bt_tx_busy = false;    // 发送忙标志

// /**
//  * @brief  GPIO 配置 PA0(TX), PA1(RX)
//  */
// static void bt_gpio_config(void)
// {
//     // 使能GPIO时钟
//     rcu_periph_clock_enable(RCU_BT_TX_GPIO);
//     rcu_periph_clock_enable(RCU_BT_RX_GPIO);
    
//     // PA0 复用推挽输出 (TX)
//     gpio_af_set(BT_TX_PORT,BT_TX_AF,BT_TX_PIN);
//     gpio_mode_set(BT_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BT_TX_PIN);
//     gpio_output_options_set(BT_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, BT_TX_PIN);
//     // PA1 浮空输入 (RX)
//     gpio_af_set(BT_RX_PORT,BT_RX_AF,BT_RX_PIN);
//     gpio_mode_set(BT_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BT_RX_PIN);
//     gpio_output_options_set(BT_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, BT_RX_PIN);
// }

// /**
//  * @brief  DMA 配置
//  */
// static void bt_dma_config(void)
// {
//     // 使能DMA和DMAMUX时钟
//     rcu_periph_clock_enable(RCU_BT_TX_DMA_PERIPH);
//     rcu_periph_clock_enable(RCU_BT_RX_DMA_PERIPH);
//     rcu_periph_clock_enable(RCU_DMAMUX);

//     dma_single_data_parameter_struct dma_init_struct;

//     dma_deinit(BT_TX_DMA_PERIPH, BT_TX_DMA_CH);
//     dma_deinit(BT_RX_DMA_PERIPH, BT_RX_DMA_CH);

//     dma_single_data_para_struct_init(&dma_init_struct);
//     // --- 配置 DMA0 Channel 0 (用于 BT_PERIPH TX) ---
//     dma_init_struct.request = BT_TX_DMA_REQUEST;
//     dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
//     dma_init_struct.memory0_addr = (uint32_t)dma_tx_buf;
//     dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
//     dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
//     dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
//     dma_init_struct.number = 0; // 初始为0，发送时再设置
//     dma_init_struct.periph_addr = (uint32_t)&USART_TDATA(UART3);
//     dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
//     dma_init_struct.priority = DMA_PRIORITY_MEDIUM;
//     dma_single_data_mode_init(BT_TX_DMA_PERIPH, BT_TX_DMA_CH, &dma_init_struct);
    
//     /* 使能发送完成中断 */
//     dma_interrupt_enable(BT_TX_DMA_PERIPH, BT_TX_DMA_CH, DMA_INT_FTF);
//     nvic_irq_enable(BT_TX_DMA_IRQn, 5, 0);

//     // --- 配置 DMA0 Channel 1 (用于 BT_PERIPH RX) ---
//     dma_init_struct.request = BT_RX_DMA_REQUEST;
//     dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
//     dma_init_struct.memory0_addr = (uint32_t)bt_dma_rxbuf1;
//     dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
//     dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
//     dma_init_struct.number = DMA_RX_BUF_SIZE; // 缓冲区大小
//     dma_init_struct.periph_addr = (uint32_t)&USART_RDATA(UART3);
//     dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
//     dma_init_struct.priority = DMA_PRIORITY_HIGH;
//     dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE; // 关闭循环模式
//     dma_single_data_mode_init(BT_RX_DMA_PERIPH, BT_RX_DMA_CH, &dma_init_struct); 
//     /* 使能接收完成中断 */
//     dma_interrupt_enable(BT_RX_DMA_PERIPH, BT_RX_DMA_CH, DMA_INT_FTF);
//     nvic_irq_enable(BT_RX_DMA_IRQn, 4, 0);
// }

// /**
//  * @brief  初始化 BT_PERIPH (包含DMA和GPIO)
//  */
// static void bt24_init(uint32_t baudrate)
// {
//     /* 1. 开启时钟 */
//     rcu_periph_clock_enable(RCU_BT_PERIPH);
//     /* 2. 配置DMA */
//     bt_dma_config();
//     /* 3. 配置GPIO */
//     bt_gpio_config();
//     /* 4. 配置BT_PERIPH */
//     usart_deinit(BT_PERIPH);
//     usart_baudrate_set(BT_PERIPH, baudrate);
//     usart_word_length_set(BT_PERIPH, USART_WL_8BIT);
//     usart_stop_bit_set(BT_PERIPH, USART_STB_1BIT);
//     usart_parity_config(BT_PERIPH, USART_PM_NONE);
//     usart_hardware_flow_rts_config(BT_PERIPH, USART_RTS_DISABLE);
//     usart_hardware_flow_cts_config(BT_PERIPH, USART_CTS_DISABLE);
//     // 开启接收和发送
//     usart_receive_config(BT_PERIPH, USART_RECEIVE_ENABLE);
//     usart_transmit_config(BT_PERIPH, USART_TRANSMIT_ENABLE);
//     // 使能DMA接收和发送
//     usart_dma_receive_config(BT_PERIPH, USART_RECEIVE_DMA_ENABLE);
//     usart_dma_transmit_config(BT_PERIPH, USART_TRANSMIT_DMA_ENABLE);
//     /* 5. 配置NVIC (中断优先级) */
//     nvic_irq_enable(BT_PERIPH_IRQn, 3, 0);
//     /* 6. 使能BT_PERIPH */
//     usart_enable(BT_PERIPH);
//     // 开启空闲中断 (IDLE Interrupt)
//     usart_interrupt_enable(BT_PERIPH, USART_INT_IDLE);
//     /* 检查是否有空闲中断（上电时误触发）有就清除 */
//     usart_interrupt_flag_clear(BT_PERIPH, USART_INT_FLAG_IDLE);
//     /* 7. 使能DMA通道 先开启接收DMA，让它时刻准备接收 */
//     dma_channel_enable(BT_RX_DMA_PERIPH, BT_RX_DMA_CH);
//     /* 告知DMA接收使用的缓冲区 */
//     dma_rxbuf = DMA_RXBUF1;
// }

// /**
//  * @brief  清空接收APP缓冲区
//  */
// static void BT24_ClearAppBuffer(void)
// {
//     memset(bt_rx_dma_appBuf, 0, BT_RX_DMA_APPBUFF_SIZE);
// }

// /**
//  * @brief  清空接收缓冲区
//  */
// static void BT24_ClearDMABuffer(uint8_t *targetBuf)
// {
//     memset(targetBuf, 0, DMA_RX_BUF_SIZE);
// }

// /**
//  * @brief  BT24 模块初始化（调用底层 bt24_init）
//  */
// void BT24_init(uint32_t baudrate)
// {
//     bt24_init(baudrate);
//     bt24_state = BT_STATE_IDLE;
// }

// /**
//  * @brief  获取模块当前状态
//  */
// bt24_state_t BT24_GetState(void)
// {
//     return bt24_state;
// }

// /*-----------------------------------------------------------
//  * 发送 AT 指令并等待期望的响应
//  * 参数：
//  *   cmd               : AT 指令字符串（例如 "AT\r\n"）
//  *   expected_response : 期望收到的响应子串（例如 "OK" 或 "+NAME:"）
//  *   timeout_ms        : 超时时间（毫秒）
//  * 返回值：
//  *   0 ：成功收到期望响应
//  *   1 ：超时或未收到
//  *-----------------------------------------------------------*/
// uint8_t BT24_SendATCmd(uint8_t *cmd, uint8_t *expected_response, uint32_t timeout_ms)
// {
//     // 清空旧数据
//     BT24_ClearAppBuffer();

//     // 通过 DMA 发送 AT 指令
//     if (bt_send_dma((uint8_t *)cmd, strlen((char*)cmd)) != 0) {
//         return 1; // 发送失败
//     }

//     // 等待响应
//     uint32_t tick_start = get_tick(); // 假设有一个获取系统tick的函数

//     while ((get_tick() - tick_start) < timeout_ms) {
//         // 更新应用缓冲区
//         BT24_UpdateAppBuffer();

//         if (bt_rxbuf1_newlen > 0) {
//             // 确保数据以'\0'结尾，便于字符串处理
//             if (bt_rxbuf1_newlen >= DMA_RX_BUF_SIZE) bt_rxbuf1_newlen = DMA_RX_BUF_SIZE - 1;
//             memcpy(parse_buffer, bt_rx_dma_appBuf, bt_rxbuf1_newlen);
//             parse_buffer[bt_rxbuf1_newlen] = '\0';

//             // 检查是否收到了期望的响应
//             if (strstr((char*)parse_buffer, (char*)expected_response) != NULL) {
//                 return 0; // 成功
//             }
//         }
//         // 简单延时，避免CPU空转
//         for (volatile int i = 0; i < 1000; i++);
//     }
//     return 1; // 超时或未收到预期响应
// }

// /*-----------------------------------------------------------
//  * 设置蓝牙设备名称
//  * 例：BT24_SetName("MyBT24");
//  *-----------------------------------------------------------*/
// void BT24_SetName(uint8_t *name)
// {
//     bsp_8080_lcd_clear(WHITE);
//     uint8_t cmd[32];
//     sprintf((char*)cmd, "AT+NAME=%s\r\n", (char*)name);
//     if (BT24_SendATCmd(cmd, (uint8_t*)"OK", BT24_CMD_TIMEOUT)){
//         bsp_8080_lcd_printf("BT24 set name failed\r\n");
//     }else{
//         bsp_8080_lcd_printf("BT24 set name succeed\r\n");
//     }
// }

// /*-----------------------------------------------------------
//  * 设置串口波特率
//  * 常见波特率：1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
//  *-----------------------------------------------------------*/
// void BT24_SetBaudrate(uint32_t baud)
// {
//     bsp_8080_lcd_clear(WHITE);
//     uint8_t cmd[16];
//     sprintf((char*)cmd, "AT+BAUD=%d\r\n", (int)baud);
//     if (BT24_SendATCmd(cmd, (uint8_t*)"OK", BT24_CMD_TIMEOUT))
//     {
//         bsp_8080_lcd_printf("set baudrate failed\r\n");
//     }
//     else
//     {
//         bsp_8080_lcd_printf("set baudrate succeed");
//     }
// }

// /*-----------------------------------------------------------
//  * 设置为主模式
//  *-----------------------------------------------------------*/
// void BT24_SetMasterMode(void)
// {
//     bsp_8080_lcd_clear(WHITE);
//     if (BT24_SendATCmd((uint8_t*)"AT+ROLE=1\r\n", (uint8_t*)"OK", BT24_CMD_TIMEOUT))
//     {
//         bsp_8080_lcd_printf("set Master failed\r\n");
//     }
//     else
//     {
//         bsp_8080_lcd_printf("set Master succeed\r\n");
//         bt24_state = BT_STATE_CMD_MODE;
//     }
// }

// /*-----------------------------------------------------------
//  * 设置为从模式（默认）
//  *-----------------------------------------------------------*/
// void BT24_SetSlaveMode(void)
// {
//     bsp_8080_lcd_clear(WHITE);
//     if (BT24_SendATCmd((uint8_t*)"AT+ROLE=0\r\n", (uint8_t*)"OK", BT24_CMD_TIMEOUT))
//     {
//         bsp_8080_lcd_printf("set Slave failed\r\n");
//     }
//     else
//     {
//         bsp_8080_lcd_printf("set Slave succeed\r\n");
//         bt24_state = BT_STATE_CMD_MODE;
//     }
// }

// /*-----------------------------------------------------------
//  * 主模式下，连接指定 MAC 地址的设备
//  * 参数 mac 格式如："12,34,56,78,9A,BC" （注意用逗号分隔）
//  *-----------------------------------------------------------*/
// void BT24_ConnectToDevice(uint8_t *mac)
// {
//     bsp_8080_lcd_clear(WHITE);
//     uint8_t cmd[32];
//     sprintf((char*)cmd, "AT+LINK=%s\r\n", (char*)mac);   // 注意：DX-BT24可能是AT+CONNECT或AT+LINK
//     if (BT24_SendATCmd(cmd, (uint8_t*)"OK", BT24_CMD_TIMEOUT))
//     {
//         bsp_8080_lcd_printf("Connect to target failed\r\n");
//     }
//     else
//     {
//         bsp_8080_lcd_printf("Connect to target succeed\r\n");
//         bt24_state = BT_STATE_CONNECTED;
//     }
// }

// /*-----------------------------------------------------------
//  * 获取固件版本信息
//  *-----------------------------------------------------------*/
// void BT24_GetVersion(void)
// {
//     bsp_8080_lcd_clear(WHITE);
//     if (BT24_SendATCmd((uint8_t*)"AT+VERSION?\r\n", (uint8_t*)"+VERSION:", BT24_CMD_TIMEOUT))
//     {
//         bsp_8080_lcd_printf("Get version failed\r\n");
//     }
//     else
//     {
//         bsp_8080_lcd_printf("Get version succeed:\r\n%s", bt_rx_dma_appBuf);
//     }
// }

// /*-----------------------------------------------------------
//  * 模块复位
//  *-----------------------------------------------------------*/
// void BT24_Reset(void)
// {
//     bsp_8080_lcd_clear(WHITE);
//     if (BT24_SendATCmd((uint8_t*)"AT+RESET\r\n", (uint8_t*)"OK", BT24_CMD_TIMEOUT))
//     {
//         bsp_8080_lcd_printf("Reset failed\r\n");
//     }
//     else
//     {
//         bsp_8080_lcd_printf("Reset succeed\r\n");
//         bt24_state = BT_STATE_IDLE;
//     }
// }

// /*-----------------------------------------------------------
//  * 进入 AT 命令模式（通常需要发送 "+++" 并等待 "OK"）
//  * 注意：DX-BT24可能需要特殊序列，这里使用通用方法
//  *-----------------------------------------------------------*/
// void BT24_EnterCmdMode(void)
// {
//     bsp_8080_lcd_clear(WHITE);
//     // 发送 "+++" 并短暂延时
//     bt_send_buffer(BT_PERIPH, (uint8_t*)"+++", 3);
//     delay_ms(100);  // 等待模块切换

//     if (BT24_SendATCmd((uint8_t*)"AT\r\n", (uint8_t*)"OK", BT24_CMD_TIMEOUT))
//     {
//         bsp_8080_lcd_printf("Enter CMD mode failed\r\n");
//     }
//     else
//     {
//         bsp_8080_lcd_printf("Enter CMD mode succeed\r\n");
//         bt24_state = BT_STATE_CMD_MODE;
//     }
// }

// /*-----------------------------------------------------------
//  * 进入透传模式
//  *-----------------------------------------------------------*/
// void BT24_EnterTransMode(void)
// {
//     bsp_8080_lcd_clear(WHITE);
//     if (BT24_SendATCmd((uint8_t*)"AT+ENTM\r\n", (uint8_t*)"OK", BT24_CMD_TIMEOUT))
//     {
//         bsp_8080_lcd_printf("Enter Trans mode failed\r\n");
//     }
//     else
//     {
//         bsp_8080_lcd_printf("Enter Trans mode succeed\r\n");
//         bt24_state = BT_STATE_TRANS_MODE;
//     }
// }

// // 其他常用 AT 指令可参照上述模式自行添加，例如：
// // AT+RESET   —— 软复位
// // AT+ORGL    —— 恢复出厂设置
// // AT+PSWD?   —— 查询配对密码
// // AT+STATE?  —— 查询当前连接状态

// /* 阻塞发送单字节 */
// void bt_send_byte(uint32_t bt_periph, uint8_t ch)
// {
//     usart_data_transmit(bt_periph, ch);
//     while (usart_flag_get(bt_periph, USART_FLAG_TBE) == RESET);
// }

// /* 阻塞发送多字节（非DMA）*/
// void bt_send_buffer(uint32_t bt_periph, uint8_t *data, uint16_t len)
// {
//     for (uint16_t i = 0; i < len; i++) {
//         bt_send_byte(bt_periph, data[i]);
//     }
// }

// /**
//  * @brief  通过DMA发送数据（非阻塞）
//  */
// int bt_send_dma(uint8_t *data, uint16_t len)
// {
//     if (bt_tx_busy || len == 0 || len > sizeof(dma_tx_buf)) {
//         return -1;
//     }

//     /* 等待当前DMA发送完成 */
//     dma_channel_disable(BT_TX_DMA_PERIPH, BT_TX_DMA_CH);
//     while ((DMA_CHCTL(BT_TX_DMA_PERIPH, BT_TX_DMA_CH)) & DMA_CHXCTL_CHEN);

//     /* 复制数据到发送缓冲区（也可直接使用原地址，这里简单复制）*/
//     for (uint16_t i = 0; i < len; i++) {
//         dma_tx_buf[i] = data[i];
//     }

//     dma_transfer_number_config(BT_TX_DMA_PERIPH, BT_TX_DMA_CH, len);
//     dma_memory_address_config(BT_TX_DMA_PERIPH, BT_TX_DMA_CH, 0U, (uint32_t)dma_tx_buf);
//     dma_periph_address_config(BT_TX_DMA_PERIPH, BT_TX_DMA_CH, BTX_TDATA_ADDRESS);

//     bt_tx_busy = true;
//     dma_channel_enable(BT_TX_DMA_PERIPH, BT_TX_DMA_CH);

//     return 0;
// }

// void process_data(uint8_t* data, uint16_t len){
//     bt_send_dma(data, len);
// }

// /* DMA 发送完成中断 */
// void BT_TX_DMA_Channel_IRQHandler(void)
// {
//     if (dma_interrupt_flag_get(BT_TX_DMA_PERIPH, BT_TX_DMA_CH, DMA_INT_FLAG_FTF)) {
//         dma_interrupt_flag_clear(BT_TX_DMA_PERIPH, BT_TX_DMA_CH, DMA_INT_FLAG_FTF);
        
//         bt_tx_busy = false;
//     }
// }

// /* DMA接收完成中断 */
// void BT_RX_DMA_Channel_IRQHandler(void){
//     /* 检查全传输完成中断 */
//     if(dma_interrupt_flag_get(BT_RX_DMA_PERIPH, BT_RX_DMA_CH, DMA_INT_FLAG_FTF) != RESET){
//         /* 清除标志位 */
//         dma_interrupt_flag_clear(BT_RX_DMA_PERIPH, BT_RX_DMA_CH, DMA_INT_FLAG_FTF);
//         /* 更新head = 当前DMA写指针 */
//         bt_dma_head = DMA_RX_BUF_SIZE - dma_transfer_number_get(BT_RX_DMA_PERIPH, BT_RX_DMA_CH);
//     }
// }

// /* 串口空闲中断 - UART3 */
// void BT_IRQHandler(void)
// {
//     // 检查是否为空闲中断 (IDLE Flag)
//     if (usart_interrupt_flag_get(BT_PERIPH, USART_INT_FLAG_IDLE) != RESET){
//         // 清除空闲中断标志位
//         usart_interrupt_flag_clear(BT_PERIPH,  USART_INT_FLAG_IDLE);
//         usart_data_receive(BT_PERIPH);   // 读DR清除标志
//         // 更新head指针
//         bt_dma_head = DMA_RX_BUF_SIZE - dma_transfer_number_get(BT_RX_DMA_PERIPH, BT_RX_DMA_CH);
//         // 设置帧传输完成标志，通知主循环处理
//         bt_dma_need_switch = 1;
//     }
// }
         
// /**
//  * @brief  更新应用缓冲区（将新数据从DMA环形缓冲区复制到线性应用缓冲区）
//  * @return 实际复制到应用缓冲区的数据长度
//  */
// uint16_t BT24_UpdateAppBuffer(void)
// {
//     BT24_ClearAppBuffer();
//     /* 检测接收帧完成标志,同时也是DMA缓冲区切换标志 */
//     if(bt_dma_need_switch) {
//         /* 清除数据帧传输完成标志 */
//         bt_dma_need_switch = 0;
//         /* 停止DMA接收 */
//         dma_channel_disable(BT_RX_DMA_PERIPH, BT_RX_DMA_CH);
//         /* 重新配置DMA接收缓冲区 */
//         dma_memory_address_config(BT_RX_DMA_PERIPH, BT_RX_DMA_CH, 0, (dma_rxbuf == DMA_RXBUF1 ? (uint32_t)bt_dma_rxbuf2 : (uint32_t)bt_dma_rxbuf1));
//         /* 重新配置DMA接收数量 */
//         dma_transfer_number_config(BT_RX_DMA_PERIPH, BT_RX_DMA_CH, DMA_RX_BUF_SIZE);
        
//         if(dma_rxbuf == DMA_RXBUF1) {
//             memcpy(bt_rx_dma_appBuf, bt_dma_rxbuf1, bt_dma_head);
//             BT24_ClearDMABuffer(bt_dma_rxbuf1);
//         } else {
//             memcpy(bt_rx_dma_appBuf, bt_dma_rxbuf2, bt_dma_head);
//             BT24_ClearDMABuffer(bt_dma_rxbuf2);
//         }
//         /* 设置重新配置后使用的接收缓冲区 */
//         dma_rxbuf = (dma_rxbuf == DMA_RXBUF1 ? DMA_RXBUF2 : DMA_RXBUF1);
//         /* 重新启动DMA接收 */
//         dma_channel_enable(BT_RX_DMA_PERIPH, BT_RX_DMA_CH);
//         return bt_dma_head;
//     }
//     return 0;
// }










#include "gd32h7xx.h"
#include <stdbool.h>
#include <stdint.h>
#include "BT24.h"

uint8_t dma_tx_buf[DMA_TX_BUF_SIZE];
uint8_t dma_rx_buf0[DMA_RX_BUF_SIZE];
uint8_t dma_rx_buf1[DMA_RX_BUF_SIZE];
volatile uint8_t  dma_full_buf_idx = 0;     // 哪个缓冲区已满 (0 或 1)
volatile uint16_t dma_full_len = 0;         // 该缓冲区的有效数据长度
volatile uint8_t  dma_frame_ready = 0;      // 等于1表示有一帧待处理
volatile uint8_t dma_active_buf = 0;        // 0: buf0 正在被 DMA 使用; 1: buf1
volatile uint8_t bt_tx_busy = false;

/**
 * @brief  GPIO 配置 PA0(TX), PA1(RX)
 */
static void bt_gpio_config(void)
{
    // 使能GPIO时钟
    rcu_periph_clock_enable(RCU_BT_TX_GPIO);
    rcu_periph_clock_enable(RCU_BT_RX_GPIO);
    
    // PA0 复用推挽输出 (TX)
    gpio_af_set(BT_TX_PORT,BT_TX_AF,BT_TX_PIN);
    gpio_mode_set(BT_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BT_TX_PIN);
    gpio_output_options_set(BT_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, BT_TX_PIN);
    // PA1 浮空输入 (RX)
    gpio_af_set(BT_RX_PORT,BT_RX_AF,BT_RX_PIN);
    gpio_mode_set(BT_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BT_RX_PIN);
    gpio_output_options_set(BT_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, BT_RX_PIN);
}

/**
 * @brief  DMA 配置
 */
static void bt_dma_config(void)
{
    // 使能DMA和DMAMUX时钟
    rcu_periph_clock_enable(RCU_BT_TX_DMA_PERIPH);
    rcu_periph_clock_enable(RCU_BT_RX_DMA_PERIPH);
    rcu_periph_clock_enable(RCU_DMAMUX);

    dma_single_data_parameter_struct dma_init_struct;

    dma_deinit(BT_TX_DMA_PERIPH, BT_TX_DMA_CH);
    dma_deinit(BT_RX_DMA_PERIPH, BT_RX_DMA_CH);

    dma_single_data_para_struct_init(&dma_init_struct);
    // --- 配置 DMA0 Channel 0 (用于 BT_PERIPH TX) ---
    dma_init_struct.request = BT_TX_DMA_REQUEST;
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr = (uint32_t)dma_tx_buf;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    dma_init_struct.number = 0; // 初始为0，发送时再设置
    dma_init_struct.periph_addr = (uint32_t)&USART_TDATA(UART3);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_MEDIUM;
    dma_single_data_mode_init(BT_TX_DMA_PERIPH, BT_TX_DMA_CH, &dma_init_struct);
    
    /* 使能发送完成中断 */
    dma_interrupt_enable(BT_TX_DMA_PERIPH, BT_TX_DMA_CH, DMA_INT_FTF);
    nvic_irq_enable(BT_TX_DMA_IRQn, 5, 0);

    // --- 配置 DMA0 Channel 1 (用于 BT_PERIPH RX) ---
    dma_init_struct.request = BT_RX_DMA_REQUEST;
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr = (uint32_t)dma_rx_buf0;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number = DMA_RX_BUF_SIZE; // 缓冲区大小
    dma_init_struct.periph_addr = (uint32_t)&USART_RDATA(UART3);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE; // 关闭循环模式
    dma_single_data_mode_init(BT_RX_DMA_PERIPH, BT_RX_DMA_CH, &dma_init_struct); 
    /* （可选）使能接收完成中断,用于检测溢出 */
    //dma_interrupt_enable(BT_RX_DMA_PERIPH, BT_RX_DMA_CH, DMA_INT_FTF);
    //nvic_irq_enable(BT_RX_DMA_IRQn, 4, 0);
}

/**
 * @brief  初始化 BT_PERIPH (包含DMA和GPIO)
 */
void bt24_init(uint32_t baudrate)
{
    /* 1. 开启时钟 */
    rcu_periph_clock_enable(RCU_BT_PERIPH);
    /* 2. 配置DMA */
    bt_dma_config();
    /* 3. 配置GPIO */
    bt_gpio_config();
    /* 4. 配置BT_PERIPH */
    usart_deinit(BT_PERIPH);
    usart_baudrate_set(BT_PERIPH, baudrate);
    usart_word_length_set(BT_PERIPH, USART_WL_8BIT);
    usart_stop_bit_set(BT_PERIPH, USART_STB_1BIT);
    usart_parity_config(BT_PERIPH, USART_PM_NONE);
    usart_hardware_flow_rts_config(BT_PERIPH, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(BT_PERIPH, USART_CTS_DISABLE);
    // 开启接收和发送
    usart_receive_config(BT_PERIPH, USART_RECEIVE_ENABLE);
    usart_transmit_config(BT_PERIPH, USART_TRANSMIT_ENABLE);
    // 使能DMA接收和发送
    usart_dma_receive_config(BT_PERIPH, USART_RECEIVE_DMA_ENABLE);
    usart_dma_transmit_config(BT_PERIPH, USART_TRANSMIT_DMA_ENABLE);
    /* 5. 配置NVIC (中断优先级) */
    nvic_irq_enable(BT_PERIPH_IRQn, 3, 0);
    /* 6. 使能BT_PERIPH */
    usart_enable(BT_PERIPH);
    // 开启空闲中断 (IDLE Interrupt)
    usart_interrupt_enable(BT_PERIPH, USART_INT_IDLE);
    /* 检查是否有空闲中断（上电时误触发）有就清除 */
    usart_interrupt_flag_clear(BT_PERIPH, USART_INT_FLAG_IDLE);
    /* 7. 使能DMA通道 先开启接收DMA，让它时刻准备接收 */
    dma_channel_enable(BT_RX_DMA_PERIPH, BT_RX_DMA_CH);    
}

/* 串口空闲中断 - UART3 */
void BT_IRQHandler(void)
{
    // 检查是否为空闲中断 (IDLE Flag)
    if (usart_interrupt_flag_get(BT_PERIPH, USART_INT_FLAG_IDLE) != RESET){
        // 清除空闲中断标志位
        usart_interrupt_flag_clear(BT_PERIPH,  USART_INT_FLAG_IDLE);
        usart_data_receive(BT_PERIPH);   // 读DR清除标志

        uint16_t remain = dma_transfer_number_get(DMA0, BT_RX_DMA_CH);
        uint16_t len = DMA_RX_BUF_SIZE - remain;
        
        dma_full_len = len;
        dma_full_buf_idx = dma_active_buf;   // 当前正在写入的缓冲区刚接收完一帧
        dma_frame_ready = 1;
    }
}

/**
 * @brief  通过DMA发送数据（非阻塞）
 */
int bt_send_dma(uint8_t *data, uint16_t len)
{
    if (bt_tx_busy || len == 0 || len > sizeof(dma_tx_buf)) {
        return -1;
    }

    /* 等待当前DMA发送完成 */
    dma_channel_disable(BT_TX_DMA_PERIPH, BT_TX_DMA_CH);
    while ((DMA_CHCTL(BT_TX_DMA_PERIPH, BT_TX_DMA_CH)) & DMA_CHXCTL_CHEN);

    /* 复制数据到发送缓冲区（也可直接使用原地址，这里简单复制）*/
    for (uint16_t i = 0; i < len; i++) {
        dma_tx_buf[i] = data[i];
    }

    dma_transfer_number_config(BT_TX_DMA_PERIPH, BT_TX_DMA_CH, len);
    dma_memory_address_config(BT_TX_DMA_PERIPH, BT_TX_DMA_CH, 0U, (uint32_t)dma_tx_buf);
    dma_periph_address_config(BT_TX_DMA_PERIPH, BT_TX_DMA_CH, BTX_TDATA_ADDRESS);

    bt_tx_busy = true;
    dma_channel_enable(BT_TX_DMA_PERIPH, BT_TX_DMA_CH);

    return 0;
}

/* DMA 发送完成中断 */
void BT_TX_DMA_Channel_IRQHandler(void)
{
    if (dma_interrupt_flag_get(BT_TX_DMA_PERIPH, BT_TX_DMA_CH, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(BT_TX_DMA_PERIPH, BT_TX_DMA_CH, DMA_INT_FLAG_FTF);
        
        bt_tx_busy = false;
    }
}