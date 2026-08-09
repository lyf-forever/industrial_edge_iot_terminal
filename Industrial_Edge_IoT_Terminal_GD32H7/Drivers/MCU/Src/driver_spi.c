/*!
 * 文件名称： driver_spi.c
 * 描    述： spi 驱动文件
 * 版本：     2023-12-03, V1.0
*/

/*
* GD32H757海棠派开发板V1.0
* 聚沃科技官网：https://www.gd32bbs.com/
* B站视频教程：https://space.bilibili.com/475462605
* CSDN文档教程：https://gd32mcu.blog.csdn.net/
* 聚沃开发板购买：https://juwo.taobao.com
* GD技术交流QQ群：859440462
* GD官方开发板、样品购买，咨询Q：87205168  咨询V：13905102619
* GD32 MCU方案移植、方案开发，咨询Q：87205168  咨询V：13905102619
* Copyright@苏州聚沃电子科技有限公司，盗版必究。
*/


#include "driver_spi.h"
#include <stdint.h>


/*!
* 说明     spi初始化
* 输入[1]  spix：spix结构体指针 
* 返回值   无
*/
void driver_spi_init(typdef_spi_struct *spix)
{
    spi_parameter_struct spi_init_struct;    
    rcu_periph_clock_enable(spix->rcu_spi_x);
    /* spi configure */
    spi_i2s_deinit(spix->spi_x);

    spix->spi_sck_gpio->speed=GPIO_OSPEED_60MHZ;
    spix->spi_mosi_gpio->speed=GPIO_OSPEED_60MHZ;
    spix->spi_miso_gpio->speed=GPIO_OSPEED_60MHZ;
    
    driver_gpio_general_init(spix->spi_cs_gpio);    
    driver_gpio_general_init(spix->spi_sck_gpio);
    driver_gpio_general_init(spix->spi_mosi_gpio);
    driver_gpio_general_init(spix->spi_miso_gpio);	

    if(spix->spi_mode==MODE_DMA)
    {
        if(spix->spi_rx_dma!=NULL)
        {
            if(spix->frame_size==SPI_DATASIZE_8BIT){
                driver_dma_com_init(spix->spi_rx_dma,(uint32_t)&SPI_RDATA(spix->spi_x),0,DMA_Width_8BIT,DMA_PERIPH_TO_MEMORY);
            }
            else if(spix->frame_size==SPI_DATASIZE_16BIT){
                driver_dma_com_init(spix->spi_rx_dma,(uint32_t)&SPI_RDATA(spix->spi_x),0,DMA_Width_16BIT,DMA_PERIPH_TO_MEMORY);
            }
            else if(spix->frame_size==SPI_DATASIZE_32BIT){
                driver_dma_com_init(spix->spi_rx_dma,(uint32_t)&SPI_RDATA(spix->spi_x),0,DMA_Width_32BIT,DMA_PERIPH_TO_MEMORY);
            }            
        }            
            
        if(spix->spi_tx_dma!=NULL)
        {
            if(spix->frame_size==SPI_DATASIZE_8BIT){
                driver_dma_com_init(spix->spi_tx_dma,(uint32_t)&SPI_TDATA(spix->spi_x),0,DMA_Width_8BIT,DMA_MEMORY_TO_PERIPH);
            }
            else if(spix->frame_size==SPI_DATASIZE_16BIT){
                driver_dma_com_init(spix->spi_tx_dma,(uint32_t)&SPI_TDATA(spix->spi_x),0,DMA_Width_16BIT,DMA_MEMORY_TO_PERIPH);
            }
            else if(spix->frame_size==SPI_DATASIZE_32BIT){
                driver_dma_com_init(spix->spi_tx_dma,(uint32_t)&SPI_TDATA(spix->spi_x),0,DMA_Width_32BIT,DMA_MEMORY_TO_PERIPH);
            }            
        }            
    }
    
    if(spix->spi_cs_gpio!=NULL)
    {
        driver_gpio_pin_set(spix->spi_cs_gpio);
    }
    
    spi_struct_para_init(&spi_init_struct);
    /* SPI3 parameter config */
    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode = spix->device_mode;
    spi_init_struct.data_size = spix->frame_size;
    spi_init_struct.clock_polarity_phase = spix->clock_polarity_phase;
    if(spix->device_mode==SPI_MASTER){
        spi_init_struct.nss = SPI_NSS_SOFT;
    }else{
        spi_init_struct.nss = SPI_NSS_HARD;        
    }
    spi_init_struct.prescale = spix->prescale;
    spi_init_struct.endian = spix->endian;
    spi_init(spix->spi_x, &spi_init_struct);
    
    /* enable SPI byte access */
    spi_byte_access_enable(spix->spi_x);
    
        /* configure SPI current data number  */
    spi_current_data_num_config(spix->spi_x, 0);

    spi_nss_output_enable(spix->spi_x);
    
    /* enable SPI3 */
    spi_enable(spix->spi_x);
    
    /* start SPI master transfer */
    spi_master_transfer_start(spix->spi_x, SPI_TRANS_START);    
}


/*!
* 说明     spi轮训发送
* 输入[1]  spix：spix结构体指针
* 输入[2]               *pbuff --- 数据缓存指针   
* 输入[3]               Length--- 数据长度   
* 返回值   通信成功标志
*/
Drv_Err driver_spi_master_poll_transmit(typdef_spi_struct *spix,uint8_t *pbuff,uint16_t length)
{
    driver_gpio_pin_reset(spix->spi_cs_gpio);
    
    spi_i2s_flag_clear(spix->spi_x, SPI_STATC_TXURERRC|SPI_STATC_RXORERRC|SPI_STATC_CRCERRC|SPI_STATC_FERRC|SPI_STATC_CONFERRC);    
       
    
    delay_us(1);
    spix->spi_control.Com_Flag.Bits.SendSuccess=0;
    spix->spi_control.Com_Flag.Bits.SendState=1;    
    spix->spi_control.p_Send=pbuff;
    spix->spi_control.SendSize=length;
    spix->spi_control.SendCount=0;
    
    for(uint16_t i=0;i<length;i++){
        if(DRV_ERROR==driver_spi_flag_wait_timeout(spix,SPI_FLAG_TP,SET))
        {
            driver_gpio_pin_set(spix->spi_cs_gpio);            
            return DRV_ERROR;
        }
        spi_i2s_data_transmit(spix->spi_x,pbuff[i]);
        spix->spi_control.SendCount++;
    }

    if(DRV_ERROR==driver_spi_flag_wait_timeout(spix,SPI_FLAG_TC,SET))
    {
        driver_gpio_pin_set(spix->spi_cs_gpio);            
        return DRV_ERROR;
    }
    
    spix->spi_control.Com_Flag.Bits.SendSuccess=1;
    spix->spi_control.Com_Flag.Bits.SendState=0;   

    delay_us(1);
    spi_i2s_flag_clear(spix->spi_x, SPI_STATC_TXURERRC|SPI_STATC_RXORERRC|SPI_STATC_CRCERRC|SPI_STATC_FERRC|SPI_STATC_CONFERRC);    
     

    driver_gpio_pin_set(spix->spi_cs_gpio);    

    return DRV_SUCCESS;	
}


/*!
* 说明     spi轮训接受
* 输入[1]  spix：spix结构体指针
* 输入[2]               *pbuff --- 数据缓存指针   
* 输入[3]               Length--- 数据长度   
* 返回值   通信成功标志
*/
Drv_Err driver_spi_master_poll_receive(typdef_spi_struct *spix,uint8_t *pbuff,uint16_t length)
{    
    driver_gpio_pin_reset(spix->spi_cs_gpio);
    
    delay_us(1);
    
    spix->spi_control.Com_Flag.Bits.RecSuccess=0;
    spix->spi_control.Com_Flag.Bits.RecState=1;
    spix->spi_control.p_Rec=pbuff;
    spix->spi_control.RecSize=length;
    spix->spi_control.RecCount=0;   

    spi_i2s_flag_clear(spix->spi_x, SPI_STATC_TXURERRC|SPI_STATC_RXORERRC|SPI_STATC_CRCERRC|SPI_STATC_FERRC|SPI_STATC_CONFERRC);    
         
    
    for(uint16_t i=0;i<length;i++){   
        if(DRV_ERROR==driver_spi_flag_wait_timeout(spix,SPI_FLAG_TP,SET))
        {
            driver_gpio_pin_set(spix->spi_cs_gpio);            
            return DRV_ERROR;
        }
        spi_i2s_data_transmit(spix->spi_x,0xff);
                
        if(DRV_ERROR==driver_spi_flag_wait_timeout(spix,SPI_FLAG_RP,SET))
        {
            driver_gpio_pin_set(spix->spi_cs_gpio);            
            return DRV_ERROR;
        }
        pbuff[i]=spi_i2s_data_receive(spix->spi_x);
        spix->spi_control.RecCount++;
    }

    if(DRV_ERROR==driver_spi_flag_wait_timeout(spix,SPI_FLAG_TC,SET))
    {
        driver_gpio_pin_set(spix->spi_cs_gpio);            
        return DRV_ERROR;
    }    
    
    spix->spi_control.Com_Flag.Bits.RecSuccess=1;
    spix->spi_control.Com_Flag.Bits.RecState=0;
    
    delay_us(1);
    spi_i2s_flag_clear(spix->spi_x, SPI_STATC_TXURERRC|SPI_STATC_RXORERRC|SPI_STATC_CRCERRC|SPI_STATC_FERRC|SPI_STATC_CONFERRC);    
     

    driver_gpio_pin_set(spix->spi_cs_gpio);      
    
    return DRV_SUCCESS;	
}

/*!
* 说明     spi轮训发送接受
* 输入[1]  spix：spix结构体指针
* 输入[2]               *txpbuff --- 发送数据缓存指针
* 输入[3]               *rxpbuff --- 接受数据缓存指针   
* 输入[4]               Length--- 数据长度   
* 返回值   通信成功标志
*/
Drv_Err driver_spi_master_poll_transmit_receive(typdef_spi_struct *spix,uint8_t *txpbuff,uint8_t *rxpbuff,uint16_t length)
{    
    driver_gpio_pin_reset(spix->spi_cs_gpio);
    
    delay_us(1);
    
    spix->spi_control.Com_Flag.Bits.RecSuccess=0;
    spix->spi_control.Com_Flag.Bits.RecState=1;
    spix->spi_control.p_Rec=rxpbuff;
    spix->spi_control.RecSize=length;
    spix->spi_control.RecCount=0;  

    spix->spi_control.Com_Flag.Bits.SendSuccess=0;
    spix->spi_control.Com_Flag.Bits.SendState=1;    
    spix->spi_control.p_Send=txpbuff;
    spix->spi_control.SendSize=length;
    spix->spi_control.SendCount=0;      

    spi_i2s_flag_clear(spix->spi_x, SPI_STATC_TXURERRC|SPI_STATC_RXORERRC|SPI_STATC_CRCERRC|SPI_STATC_FERRC|SPI_STATC_CONFERRC);    
         
    
    for(uint16_t i=0;i<length;i++){
    
        if(DRV_ERROR==driver_spi_flag_wait_timeout(spix,SPI_FLAG_TP,SET))
        {
            driver_gpio_pin_set(spix->spi_cs_gpio);            
            return DRV_ERROR;
        }
        spi_i2s_data_transmit(spix->spi_x,txpbuff[i]);
        spix->spi_control.SendCount++;
    
        if(DRV_ERROR==driver_spi_flag_wait_timeout(spix,SPI_FLAG_RP,SET))
        {
            driver_gpio_pin_set(spix->spi_cs_gpio);
            return DRV_ERROR;
        }
        rxpbuff[i]=spi_i2s_data_receive(spix->spi_x);
        spix->spi_control.RecCount++;
    }

    if(DRV_ERROR==driver_spi_flag_wait_timeout(spix,SPI_FLAG_TC,SET))
    {
            driver_gpio_pin_set(spix->spi_cs_gpio);        
            return DRV_ERROR;
    }    
    
    spix->spi_control.Com_Flag.Bits.RecSuccess=1;
    spix->spi_control.Com_Flag.Bits.RecState=0;
    
    spix->spi_control.Com_Flag.Bits.SendSuccess=1;
    spix->spi_control.Com_Flag.Bits.SendState=0;       
    
    delay_us(1);
    spi_i2s_flag_clear(spix->spi_x, SPI_STATC_TXURERRC|SPI_STATC_RXORERRC|SPI_STATC_CRCERRC|SPI_STATC_FERRC|SPI_STATC_CONFERRC);    
     
    driver_gpio_pin_set(spix->spi_cs_gpio);      
    
    return DRV_SUCCESS;	
}


/*!
* 说明     spi轮训发送接受一个数据
* 输入[1]  spix：spix结构体指针
* 输入[2]  byte--- 发送数据  
* 返回值   接受数据
*/
uint8_t driver_spi_master_transmit_receive_byte(typdef_spi_struct *spix,uint8_t byte)
{
    
    spi_i2s_flag_clear(spix->spi_x, SPI_STATC_TXURERRC|SPI_STATC_RXORERRC|SPI_STATC_CRCERRC|SPI_STATC_FERRC|SPI_STATC_CONFERRC);    
             
    driver_spi_flag_wait_timeout(spix,SPI_FLAG_TP,SET);

    spi_i2s_data_transmit(spix->spi_x,byte);

    driver_spi_flag_wait_timeout(spix,SPI_FLAG_RP,SET);

    return spi_i2s_data_receive(spix->spi_x);                
}


/*!
* 说明     spi dma发送
* 输入[1]  spix：spix结构体指针
* 输入[2]               *pbuff --- 发送数据缓存指针 
* 输入[3]               Length--- 数据长度   
* 返回值   通信成功标志
*/
Drv_Err driver_spi_master_dma_transmit(typdef_spi_struct *spix,uint8_t *pbuff,uint16_t length)
{
    Drv_Err spi_state=DRV_ERROR;
    
    spi_state=driver_spi_flag_wait_timeout(spix,SPI_FLAG_TC,SET);    
    driver_gpio_pin_set(spix->spi_cs_gpio);

    spi_i2s_flag_clear(spix->spi_x, SPI_STATC_TXURERRC|SPI_STATC_RXORERRC|SPI_STATC_CRCERRC|SPI_STATC_FERRC|SPI_STATC_CONFERRC);    
          
    
    spix->spi_control.Com_Flag.Bits.SendSuccess=0;
    spix->spi_control.Com_Flag.Bits.SendState=1;    
    spix->spi_control.p_Send=pbuff;
    spix->spi_control.SendSize=length;
    spix->spi_control.SendCount=0;
       
    driver_gpio_pin_reset(spix->spi_cs_gpio);  
    delay_us(1);
    
    spi_dma_disable(spix->spi_x,SPI_DMA_TRANSMIT);   
    
    driver_dma_start(spix->spi_tx_dma,pbuff,length);
    
    spi_dma_enable(spix->spi_x,SPI_DMA_TRANSMIT);

    return spi_state;   
}

/*!
* 说明     spi dma接受
* 输入[1]  spix：spix结构体指针
* 输入[2]               *pbuff --- 接受数据缓存指针   
* 输入[3]               Length--- 数据长度   
* 返回值   通信成功标志
*/
Drv_Err driver_spi_master_dma_receive(typdef_spi_struct *spix,uint8_t *pbuff,uint16_t length)
{
    Drv_Err spi_state=DRV_ERROR;
    
    spi_state=driver_spi_flag_wait_timeout(spix,SPI_FLAG_TC,SET);    
    driver_gpio_pin_set(spix->spi_cs_gpio);

    spi_i2s_flag_clear(spix->spi_x, SPI_STATC_TXURERRC|SPI_STATC_RXORERRC|SPI_STATC_CRCERRC|SPI_STATC_FERRC|SPI_STATC_CONFERRC);    
          
    
    spix->spi_control.Com_Flag.Bits.RecSuccess=0;
    spix->spi_control.Com_Flag.Bits.RecState=1;
    spix->spi_control.p_Rec=pbuff;
    spix->spi_control.RecSize=length;
    spix->spi_control.RecCount=0;         
    
    driver_gpio_pin_reset(spix->spi_cs_gpio); 
    delay_us(1);           
    
//    spi_dma_disable(spix->spi_x,SPI_DMA_TRANSMIT); 
//    spi_dma_disable(spix->spi_x,SPI_DMA_RECEIVE); 
    
    driver_dma_start(spix->spi_rx_dma,pbuff,length);    
    driver_dma_start(spix->spi_tx_dma,pbuff,length);
    
    spi_dma_enable(spix->spi_x,SPI_DMA_RECEIVE);    
    spi_dma_enable(spix->spi_x,SPI_DMA_TRANSMIT);
    
    

    return spi_state;   
}


/*!
* 说明     spi dma发送接受
* 输入[1]  spix：spix结构体指针
* 输入[2]               *txpbuff --- 发送数据缓存指针
* 输入[3]               *rxpbuff --- 接受数据缓存指针   
* 输入[4]               Length--- 数据长度   
* 返回值   通信成功标志
*/
Drv_Err driver_spi_master_dma_transmit_receive(typdef_spi_struct *spix,uint8_t *txpbuff,uint8_t *rxpbuff,uint16_t length)
{    
    Drv_Err spi_state=DRV_ERROR;
                    
    spi_state=driver_spi_flag_wait_timeout(spix,SPI_FLAG_TC,SET);
    driver_gpio_pin_set(spix->spi_cs_gpio);

    spix->spi_control.Com_Flag.Bits.RecSuccess=0;
    spix->spi_control.Com_Flag.Bits.RecState=1;
    spix->spi_control.p_Rec=rxpbuff;
    spix->spi_control.RecSize=length;
    spix->spi_control.RecCount=0;  

    spix->spi_control.Com_Flag.Bits.SendSuccess=0;
    spix->spi_control.Com_Flag.Bits.SendState=1;    
    spix->spi_control.p_Send=txpbuff;
    spix->spi_control.SendSize=length;
    spix->spi_control.SendCount=0;      
    
        
    spi_i2s_flag_clear(spix->spi_x, SPI_STATC_TXURERRC|SPI_STATC_RXORERRC|SPI_STATC_CRCERRC|SPI_STATC_FERRC|SPI_STATC_CONFERRC);              
    
    driver_gpio_pin_reset(spix->spi_cs_gpio);
    delay_us(1);
    
    spi_dma_disable(spix->spi_x,SPI_DMA_TRANSMIT); 
    spi_dma_disable(spix->spi_x,SPI_DMA_RECEIVE); 
    
    driver_dma_start(spix->spi_rx_dma,rxpbuff,length);    
    driver_dma_start(spix->spi_tx_dma,txpbuff,length);
    
    spi_dma_enable(spix->spi_x,SPI_DMA_RECEIVE);    
    spi_dma_enable(spix->spi_x,SPI_DMA_TRANSMIT);

    return spi_state;   
}



/*!
* 说明     spi dma从机发送接受
* 输入[1]  spix：spix结构体指针
* 输入[2]               *txpbuff --- 发送数据缓存指针
* 输入[3]               *rxpbuff --- 接受数据缓存指针   
* 输入[4]               Length--- 数据长度   
* 返回值   通信成功标志
*/
Drv_Err driver_spi_slave_dma_transmit_receive(typdef_spi_struct *spix,uint8_t *txpbuff,uint8_t *rxpbuff,uint16_t length)
{
    Drv_Err spi_state=DRV_ERROR;
    
    spi_state=driver_spi_flag_wait_timeout(spix,SPI_FLAG_TC,SET);    
        
    spix->spi_control.Com_Flag.Bits.RecSuccess=0;
    spix->spi_control.Com_Flag.Bits.RecState=1;
    spix->spi_control.p_Rec=rxpbuff;
    spix->spi_control.RecSize=length;
    spix->spi_control.RecCount=0;  

    spix->spi_control.Com_Flag.Bits.SendSuccess=0;
    spix->spi_control.Com_Flag.Bits.SendState=1;    
    spix->spi_control.p_Send=txpbuff;
    spix->spi_control.SendSize=length;
    spix->spi_control.SendCount=0;      

    spi_i2s_flag_clear(spix->spi_x, SPI_STATC_TXURERRC|SPI_STATC_RXORERRC|SPI_STATC_CRCERRC|SPI_STATC_FERRC|SPI_STATC_CONFERRC);    

    spi_dma_disable(spix->spi_x,SPI_DMA_TRANSMIT); 
    spi_dma_disable(spix->spi_x,SPI_DMA_RECEIVE); 
    
    driver_dma_start(spix->spi_rx_dma,rxpbuff,length);    
    driver_dma_start(spix->spi_tx_dma,txpbuff,length);
   
    spi_dma_enable(spix->spi_x,SPI_DMA_TRANSMIT);   
    spi_dma_enable(spix->spi_x,SPI_DMA_RECEIVE);    

    return spi_state; 
}




//Drv_Err driver_spi_int_transmit(typdef_spi_struct *spix,uint8_t *pbuff,uint16_t length)
//{
//    spix->spi_control.Com_Flag.Bits.SendSuccess=0;
//    spix->spi_control.Com_Flag.Bits.SendState=1;  
//    spix->spi_control.p_Send=pbuff;
//    spix->spi_control.SendSize=length;
//    spix->spi_control.SendCount=0;
//    
//    spi_flag_clear(spix->spi_x,spi_FLAG_TC);
//    spi_interrupt_enable(spix->spi_x,spi_INT_TBE);
//    
//    return DRV_SUCCESS;    
//}


//Drv_Err driver_spi_int_receive(typdef_spi_struct *spix,uint8_t *pbuff,uint16_t length)
//{
//    spix->spi_control.Com_Flag.Bits.RecSuccess=0;
//    spix->spi_control.Com_Flag.Bits.RecState=1;
//    spix->spi_control.p_Rec=pbuff;
//    spix->spi_control.RecSize=length;
//    spix->spi_control.RecCount=0;    

//    spi_flag_clear(spix->spi_x,spi_FLAG_IDLE);
//     
//    spi_i2s_flag_clear(spix->spi_x, SPI_STATC_TXURERRC|SPI_STATC_RXORERRC|SPI_STATC_CRCERRC|SPI_STATC_FERRC|SPI_STATC_CONFERRC);      
//    
//    spi_interrupt_enable(spix->spi_x,spi_INT_RBNE);
//    spi_interrupt_enable(spix->spi_x,spi_INT_IDLE);
//  
//    
//    return DRV_SUCCESS;    
//}


/*!
* 说明     超时等待flag状态
* 输入[1]  spix：spix结构体指针
* 输入[2]  flag：等待的flag
* 输入[3]  wait_state：等待的flag状态 
* 返回值   无
*/
Drv_Err driver_spi_flag_wait_timeout(typdef_spi_struct *spix, uint32_t flag ,FlagStatus wait_state)
{
    __IO uint64_t timeout = driver_tick;    
    while(wait_state!=spi_i2s_flag_get(spix->spi_x, flag)){
        if((timeout+SPI_TIMEOUT_MS) <= driver_tick) {              
            return DRV_ERROR;
        } 
    }
    return DRV_SUCCESS;
}

/*
Drv_Err driver_spi_int_handler(typdef_spi_struct *spix)
{   
   Drv_Err spi_state=DRV_SUCCESS;    
   if(spi_interrupt_flag_get(spix->spi_x,spi_INT_FLAG_RBNE)!=RESET)
   {
       if(spix->spi_control.RecCount < spix->spi_control.RecSize){
           spix->spi_control.p_Rec[spix->spi_control.RecCount]=spi_i2s_data_receive(spix->spi_x); 
           spix->spi_control.RecCount++;            
       }
       else{
           spi_i2s_data_receive(spix->spi_x);
           spi_state=DRV_ERROR;
           //err 溢出
       }
       if(spix->spi_rbne_callback!=NULL){
           spix->spi_rbne_callback(spix);
       }        
       //callback
       if(spix->spi_control.RecCount == spix->spi_control.RecSize){
           spix->spi_control.Com_Flag.Bits.RecSuccess=1;            
           spix->spi_control.Com_Flag.Bits.RecState=0; 
           spix->spi_control.RecCount=0;            
       }        
   }       
   if(spi_interrupt_flag_get(spix->spi_x,spi_INT_FLAG_IDLE)!=RESET)
   {
       spi_interrupt_flag_clear(spix->spi_x,spi_INT_FLAG_IDLE);
        
       spi_i2s_flag_clear(spix->spi_x, SPI_STATC_TXURERRC|SPI_STATC_RXORERRC|SPI_STATC_CRCERRC|SPI_STATC_FERRC|SPI_STATC_CONFERRC);    
       
       if( (spix->spi_mode_rx==MODE_INT && spix->spi_control.RecCount>0) \
           ||(spix->spi_mode_rx==MODE_DMA && dma_transfer_number_get(spix->spi_rx_dma->dmax,spix->spi_rx_dma->dma_chx)!=spix->spi_control.RecSize))
       {
           spix->spi_control.Com_Flag.Bits.RecSuccess=1;
           if(spix->spi_mode_rx==MODE_DMA){
               spix->spi_control.RecCount=dma_transfer_number_get(spix->spi_rx_dma->dmax,spix->spi_rx_dma->dma_chx);            
           }
           //callback            
           if(spix->spi_idle_callback!=NULL){
               spix->spi_idle_callback(spix);
           }           
           spix->spi_control.Com_Flag.Bits.RecState=0; 
           spix->spi_control.RecCount=0;            
       }        
           
   } 
   
   if(spi_interrupt_flag_get(spix->spi_x,spi_INT_FLAG_TBE)!=RESET)
   {
       spi_data_transmit(spix->spi_x,spix->spi_control.p_Send[spix->spi_control.SendCount]);
       spix->spi_control.SendCount++;
       
       if(spix->spi_tbe_callback!=NULL){
           spix->spi_tbe_callback(spix);
       } 
       
       if(spix->spi_control.SendCount >= spix->spi_control.SendSize)
       {
           spix->spi_control.SendCount=0;            
           spi_interrupt_disable(spix->spi_x, spi_INT_TBE);
           spi_interrupt_enable(spix->spi_x, spi_INT_TC);
       }
   } 

   if(spi_interrupt_flag_get(spix->spi_x,spi_INT_FLAG_TC)!=RESET)
   {
       spi_interrupt_disable(spix->spi_x, spi_INT_TC);          
       spi_flag_clear(spix->spi_x,spi_FLAG_TC);      
       spix->spi_control.Com_Flag.Bits.SendSuccess=1;            
       spix->spi_control.Com_Flag.Bits.SendState=0; 
       
       if(spix->spi_tc_callback!=NULL){
           spix->spi_tc_callback(spix);
       }         
       
       spix->spi_control.SendCount=0; 
   }  

   if(spi_flag_get(spix->spi_x,spi_FLAG_ORERR)==SET)
   {
       spi_flag_clear(spix->spi_x,spi_FLAG_ORERR);        
        
       spi_i2s_flag_clear(spix->spi_x, SPI_STATC_TXURERRC|SPI_STATC_RXORERRC|SPI_STATC_CRCERRC|SPI_STATC_FERRC|SPI_STATC_CONFERRC);    
       spi_state=DRV_ERROR;
   }

   return spi_state;     

}
*/




