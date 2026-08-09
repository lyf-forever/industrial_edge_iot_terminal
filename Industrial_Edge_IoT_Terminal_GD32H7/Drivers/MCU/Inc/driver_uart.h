/*!
 * 文件名称： driver_uart.h
 * 描    述： uart 驱动文件
 * 版本：     2023-12-03, V1.0
*/

/*
* GD32H757海棠派开发板V1.0
* 淘宝商城：   https://juwo.taobao.com
* 技术交流：   https://www.gd32bbs.com/ask/
* 视频学习：   https://space.bilibili.com/475462605
* 微信公众号： gd32bbs
* Copyright    苏州聚沃电子科技有限公司
* 版权所有，盗版必究。
*/

#ifndef DRIVER_UART_H
#define DRIVER_UART_H
#include "public.h"

#define UART_TIMEOUT_MS  200

typedef struct __typdef_uart_struct
{  
    rcu_periph_enum rcu_uart_x; 
    uint32_t uart_x; 
    uint32_t baudrate;
    uint32_t parity;
    uint32_t data_length;
    com_mode_enum uart_mode_tx; //UART_POLL,UART_DMA,UART_INT 
    com_mode_enum uart_mode_rx; //UART_POLL,UART_DMA,UART_INT     

    typdef_gpio_general* uart_tx_gpio;
    typdef_gpio_general* uart_rx_gpio;  
    typdef_dma_general*  uart_tx_dma;
    typdef_dma_general*  uart_rx_dma;  

    typdef_com_control_general  uart_control;

    void (*uart_tbe_callback)(struct  __typdef_uart_struct *uartx);
    void (*uart_tc_callback)(struct   __typdef_uart_struct *uartx);
    void (*uart_rbne_callback)(struct __typdef_uart_struct *uartx);
    void (*uart_idle_callback)(struct __typdef_uart_struct *uartx);
}typdef_uart_struct;   

#define UART_TX_DMA_DEF(name,USARTx,dmax,dma_chx)       \
        typdef_dma_general name##_USART_TX_DMA = \
        {RCU_##dmax,dmax,dma_chx,Circulation_Disable,DMA_REQUEST_##USARTx##_TX,NULL}
//        DMA_DEF(name##_USART_TX_DMA,dma,dma_chx)

#define UART_RX_DMA_DEF(name,USARTx,dmax,dma_chx)       \
        typdef_dma_general name##_USART_RX_DMA = \
        {RCU_##dmax,dmax,dma_chx,Circulation_Disable,DMA_REQUEST_##USARTx##_RX,NULL}
//                DMA_DEF(name##_USART_RX_DMA,dmax,dma_chx)
        

#define UART_TX_GPIO_DEF(name, port, pin, afio_mode)      \
        AFIO_DEF(name##_USART_TX_GPIO,port,pin,AF_PP,afio_mode) 

#define UART_RX_GPIO_DEF(name, port, pin, afio_mode)      \
        AFIO_DEF(name##_USART_RX_GPIO,port,pin,IN_PU,afio_mode)  

#define UART_DEF(name, USARTx, baudrate, parity, data_length,uart_mode_tx,uart_mode_rx)      \
        typdef_uart_struct name = \
        {RCU_##USARTx,USARTx,baudrate,parity,data_length,uart_mode_tx,uart_mode_rx,&name##_USART_TX_GPIO,&name##_USART_RX_GPIO,&name##_USART_TX_DMA,&name##_USART_RX_DMA,NULL,NULL,NULL,NULL}        


#define UART_DEF_EXTERN(name) \
    extern typdef_uart_struct name


//外部函数调用
void driver_uart_init(typdef_uart_struct *uartx);
Drv_Err driver_uart_poll_transmit(typdef_uart_struct *uartx,uint8_t *pbuff,uint16_t length);
Drv_Err driver_uart_poll_receive(typdef_uart_struct *uartx,uint8_t *pbuff,uint16_t length);
Drv_Err driver_uart_dma_transmit(typdef_uart_struct *uartx,uint8_t *pbuff,uint16_t length);
Drv_Err driver_uart_transmit_byte(typdef_uart_struct *uartx,uint8_t data);
Drv_Err driver_uart_dma_receive(typdef_uart_struct *uartx,uint8_t *pbuff,uint16_t length);
Drv_Err driver_uart_int_transmit(typdef_uart_struct *uartx,uint8_t *pbuff,uint16_t length);
Drv_Err driver_uart_int_receive(typdef_uart_struct *uartx,uint8_t *pbuff,uint16_t length);
Drv_Err driver_uart_flag_wait_timeout(typdef_uart_struct *uartx, usart_flag_enum flag ,FlagStatus wait_state);
Drv_Err driver_uart_int_handler(typdef_uart_struct *uartx);

#endif /* DRIVER_UART_H*/
