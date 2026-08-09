/*!
 * 文件名称： driver_spi.h
 * 描    述： spi 驱动文件
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
#ifndef DRIVER_SPI_H
#define DRIVER_SPI_H
#include "public.h"

#define SPI_TIMEOUT_MS  100

typedef struct __typdef_spi_struct
{  
    rcu_periph_enum rcu_spi_x; 
    uint32_t spi_x; 
    uint32_t device_mode;
    uint32_t frame_size;
    uint32_t clock_polarity_phase;
    uint32_t prescale;
    uint32_t endian;  
    
    com_mode_enum spi_mode; //UART_POLL,UART_DMA,UART_INT 
 

    typdef_gpio_general* spi_sck_gpio;
    typdef_gpio_general* spi_mosi_gpio;  
    typdef_gpio_general* spi_miso_gpio; 
    typdef_gpio_general* spi_cs_gpio;
    
    typdef_dma_general*  spi_tx_dma;
    typdef_dma_general*  spi_rx_dma;  

    typdef_com_control_general  spi_control;

    void (*spi_slave_trans_end)(struct  __typdef_spi_struct *spix);
    void (*spi_master_trans_end)(struct   __typdef_spi_struct *spix);
    void (*uart_rbne_callback)(struct __typdef_spi_struct *spix);
    void (*uart_tbe_callback)(struct __typdef_spi_struct *spix);
}typdef_spi_struct;   


#define SPI_TX_DMA_DEF(name,SPIx,dmax, dma_chx)       \
        typdef_dma_general name##_SPI_TX_DMA = \
        {RCU_##dmax,dmax,dma_chx,Circulation_Disable,DMA_REQUEST_##SPIx##_TX,NULL}     

#define SPI_RX_DMA_DEF(name,SPIx,dmax, dma_chx)       \
        typdef_dma_general name##_SPI_RX_DMA = \
        {RCU_##dmax,dmax,dma_chx,Circulation_Disable,DMA_REQUEST_##SPIx##_RX,NULL}
        

#define SPI_MASTER_SCK_GPIO_DEF(name, port, pin, afio_mode)      \
        AFIO_DEF(name##_SPI_SCK_GPIO,port,pin,AF_PP,afio_mode) 

#define SPI_MASTER_MOSI_GPIO_DEF(name, port, pin, afio_mode)      \
        AFIO_DEF(name##_SPI_MOSI_GPIO,port,pin,AF_PP,afio_mode)  
        
#define SPI_MASTER_MISO_GPIO_DEF(name, port, pin, afio_mode)      \
        AFIO_DEF(name##_SPI_MISO_GPIO,port,pin,AF_PP,afio_mode)  

#define SPI_MASTER_CS_GPIO_DEF(name, port, pin)  \
        typdef_gpio_general name##_SPI_CS_GPIO = \
        {RCU_GPIO##port,GPIO##port,GPIO_PIN_##pin,OUT_PP,GPIO_OSPEED_12MHZ,SET,EXTI_SOURCE_GPIO##port,EXTI_SOURCE_PIN##pin,EXTI_##pin,NULL,NULL}   


#define SPI_SLAVE_SCK_GPIO_DEF(name, port, pin, afio_mode)      \
        AFIO_DEF(name##_SPI_SCK_GPIO,port,pin,IN_NONE,afio_mode) 

#define SPI_SLAVE_MOSI_GPIO_DEF(name, port, pin, afio_mode)      \
        AFIO_DEF(name##_SPI_MOSI_GPIO,port,pin,IN_NONE,afio_mode)  
        
#define SPI_SLAVE_MISO_GPIO_DEF(name, port, pin, afio_mode)      \
        AFIO_DEF(name##_SPI_MISO_GPIO,port,pin,AF_PP,afio_mode)  
        
#define SPI_SLAVE_CS_GPIO_DEF(name, port, pin, afio_mode,spi_slave_cs_high_callback)  \
        typdef_gpio_general name##_SPI_CS_GPIO = \
        {RCU_GPIO##port,GPIO##port,GPIO_PIN_##pin,AF_OD,GPIO_OSPEED_12MHZ,SET,EXTI_SOURCE_GPIO##port,EXTI_SOURCE_PIN##pin,EXTI_##pin,spi_slave_cs_high_callback,afio_mode}        

#define SPI_DEF(name,SPIx,device_mode,frame_size,clock_polarity_phase,prescale,endian,spi_mode)      \
        typdef_spi_struct name = \
        {RCU_##SPIx,SPIx,device_mode,frame_size,clock_polarity_phase,prescale,endian,spi_mode,&name##_SPI_SCK_GPIO,&name##_SPI_MOSI_GPIO,&name##_SPI_MISO_GPIO,&name##_SPI_CS_GPIO,&name##_SPI_TX_DMA,&name##_SPI_RX_DMA,NULL,NULL,NULL,NULL}        


#define SPI_DEF_EXTERN(name) \
    extern typdef_spi_struct name


//外部函数调用
void driver_spi_init(typdef_spi_struct *spix);

Drv_Err driver_spi_master_poll_transmit(typdef_spi_struct *spix,uint8_t *pbuff,uint16_t length);

Drv_Err driver_spi_master_poll_receive(typdef_spi_struct *spix,uint8_t *pbuff,uint16_t length);

Drv_Err driver_spi_master_poll_transmit_receive(typdef_spi_struct *spix,uint8_t *txpbuff,uint8_t *rxpbuff,uint16_t length);

uint8_t driver_spi_master_transmit_receive_byte(typdef_spi_struct *spix,uint8_t byte);

Drv_Err driver_spi_master_dma_transmit(typdef_spi_struct *spix,uint8_t *pbuff,uint16_t length);
//    
Drv_Err driver_spi_master_dma_receive(typdef_spi_struct *spix,uint8_t *pbuff,uint16_t length);

Drv_Err driver_spi_master_dma_transmit_receive(typdef_spi_struct *spix,uint8_t *txpbuff,uint8_t *rxpbuff,uint16_t length);
    
Drv_Err driver_spi_slave_dma_transmit_receive(typdef_spi_struct *spix,uint8_t *txpbuff,uint8_t *rxpbuff,uint16_t length);




Drv_Err driver_spi_flag_wait_timeout(typdef_spi_struct *spix, uint32_t flag ,FlagStatus wait_state);
    


#endif /* GPIO_H*/
