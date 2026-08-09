/*!
 * 文件名称： driver_dma.h
 * 描    述： dma 驱动文件
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

#ifndef DRIVER_DMA_H
#define DRIVER_DMA_H

#include "public.h"
#define  DMA_TIMEOUT_MS  300U
typedef enum 
{
    DMA_Width_8BIT=0,
    DMA_Width_16BIT,
    DMA_Width_32BIT
}dma_width_enum;
typedef enum 
{
    Circulation_Disable=0,
    Circulation_Enable
}dma_circulation_enum;
typedef struct __typdef_dma_general
{
    rcu_periph_enum rcu_dmax;
    uint32_t dmax;
    dma_channel_enum dma_chx;
    //uint32_t DMA_SUBPERIx;
    dma_circulation_enum circulation;
    uint32_t request;
    void (*Complete_callback)(struct __typdef_dma_general *dma);
}typdef_dma_general;

/*
#define DMA_DEF(name, dma, dma_chx)       \
       typdef_dma_general name =  \
       {dma,dma_chx,NULL}        
*/
        
#define DMA_DEF(name, dmax, dma_chx,circulation,request)       \
        typdef_dma_general name = \
        {RCU_##dmax,dmax,dma_chx,circulation,request,NULL}
        
#define DMA_DEF_EXTERN(name) \
    extern typdef_dma_general name        
        
DMA_DEF_EXTERN(DMA_MEM);
    
void driver_dma_com_init(typdef_dma_general* dmax,uint32_t periph_addr,uint32_t memory_addr,dma_width_enum dma_width,uint32_t dma_direction);
void driver_dma_start(typdef_dma_general* dmax,uint8_t *pbuff,uint16_t length);
Drv_Err driver_dma_flag_wait_timeout(typdef_dma_general* dmax, uint32_t flag ,FlagStatus wait_state);

void driver_dma_mem_init(typdef_dma_general* dmax);        
void driver_dma_mem_to_mem_start(void* memory_dest_addr,void* memory_src_addr,dma_width_enum dma_width,uint32_t length);        
void driver_dma_mem_to_exmclcd_start(void* memory_dest_addr,void* memory_src_addr,dma_width_enum dma_width,uint32_t length);


#endif /* DRIVER_DMA_H*/
