/*!
 * 文件名称： driver_dma.c
 * 描    述： dma 驱动文件
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

#include "driver_dma.h"
#include <stdint.h>

static const uint32_t dma_width_mem[3]={DMA_MEMORY_WIDTH_8BIT,DMA_MEMORY_WIDTH_16BIT,DMA_MEMORY_WIDTH_32BIT};
static const uint32_t dma_width_per[3]={DMA_PERIPH_WIDTH_8BIT,DMA_PERIPH_WIDTH_16BIT,DMA_PERIPH_WIDTH_32BIT};

DMA_DEF(DMA_MEM,DMA1,DMA_CH2,Circulation_Disable,DMA_REQUEST_M2M);

/*!
* 说明     dma初始化
* 输入[1]  dmax：dmax结构体指针 
* 输入[2]  periph_addr：外设地址 
* 输入[3]  memory_addr：内存地址
* 输入[4]  dma_width：传输宽度
* 输入[4]  dma_direction：传输方向 @DMA_PERIPH_TO_MEMORY/DMA_MEMORY_TO_PERIPH/DMA_MEMORY_TO_MEMORY 
* 返回值   无
*/
void driver_dma_com_init(typdef_dma_general* dmax,uint32_t periph_addr,uint32_t memory_addr,dma_width_enum dma_width,uint32_t dma_direction)
{
    dma_single_data_parameter_struct dma_init_struct;

    /* enable DMA clock */
    rcu_periph_clock_enable(dmax->rcu_dmax);
    
    /* enable DMAMUX clock */
    rcu_periph_clock_enable(RCU_DMAMUX);    

    /* deinitialize DMA channel */
    dma_deinit(dmax->dmax, dmax->dma_chx);

    dma_single_data_para_struct_init(&dma_init_struct);
    
    dma_init_struct.request   = dmax->request;
    dma_init_struct.direction = dma_direction;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = dma_width_per[dma_width];
    dma_init_struct.number = 0;
    dma_init_struct.periph_addr = periph_addr;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
//    dma_init_struct.periph_width = dma_width_per[dma_width];
    if(dma_direction==DMA_PERIPH_TO_MEMORY)
    {
        dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    }else
    {
        dma_init_struct.priority = DMA_PRIORITY_LOW;
    }
    dma_single_data_mode_init(dmax->dmax, dmax->dma_chx, &dma_init_struct);

//    dma_channel_subperipheral_select(dmax->dmax, dmax->dma_chx,dmax->dma_subper);
    
    if(dmax->circulation==Circulation_Disable)
    {
        dma_circulation_disable(dmax->dmax, dmax->dma_chx);
    }else{
        dma_circulation_enable(dmax->dmax, dmax->dma_chx);
    }  
}


/*!
* 说明     dma内存模式初始化
* 输入[1]  dmax：dmax结构体指针 
* 输入[2]  periph_addr：外设地址 
* 输入[3]  memory_addr：内存地址
* 输入[4]  dma_width：传输宽度
* 输入[4]  dma_direction：传输方向 
* 返回值   无
*/
void driver_dma_mem_init(typdef_dma_general* dmax)
{
    driver_dma_com_init(dmax,0,0,DMA_Width_8BIT,DMA_MEMORY_TO_MEMORY);
}


/*!
* 说明     dma内存到内存启动
* 输入[1]  dmax：dmax结构体指针 
* 输入[2]  periph_addr：外设地址 
* 输入[3]  memory_addr：内存地址
* 输入[4]  dma_width：传输宽度
* 输入[4]  dma_direction：传输方向 
* 返回值   无
*/
void driver_dma_mem_to_mem_start(void* memory_dest_addr,void* memory_src_addr,dma_width_enum dma_width,uint32_t length)
{    
    __IO uint32_t count=0;
    const uint8_t size[3]={1,2,4};
//    driver_dma_flag_wait_timeout(&DMA_MEM,DMA_FLAG_FTF,SET);    
    dma_channel_disable(DMA_MEM.dmax,DMA_MEM.dma_chx);
    
    dma_transfer_direction_config(DMA_MEM.dmax,DMA_MEM.dma_chx,DMA_MEMORY_TO_MEMORY);
    
    
    dma_memory_width_config(DMA_MEM.dmax,DMA_MEM.dma_chx,dma_width_mem[dma_width]);
    dma_periph_width_config(DMA_MEM.dmax,DMA_MEM.dma_chx,dma_width_per[dma_width]);
    
//    dma_periph_increase_enable(DMA_MEM.dmax,DMA_MEM.dma_chx);
//    dma_memory_increase_enable(DMA_MEM.dmax,DMA_MEM.dma_chx);
    dma_peripheral_address_generation_config(DMA_MEM.dmax,DMA_MEM.dma_chx,DMA_PERIPH_INCREASE_ENABLE);
    dma_memory_address_generation_config(DMA_MEM.dmax,DMA_MEM.dma_chx,DMA_MEMORY_INCREASE_ENABLE);    
    
    dma_priority_config(DMA_MEM.dmax,DMA_MEM.dma_chx,DMA_PRIORITY_LOW);
    
    dma_circulation_disable(DMA_MEM.dmax, DMA_MEM.dma_chx);
    
//    dma_memory_to_memory_enable(DMA_MEM.dmax, DMA_MEM.dma_chx);
    
    do{  
        if(count!=0){        
//            driver_dma_flag_wait_timeout(&DMA_MEM,DMA_FLAG_FTF,SET);
            dma_channel_disable(DMA_MEM.dmax,DMA_MEM.dma_chx);        
        }
        
        dma_memory_address_config(DMA_MEM.dmax,DMA_MEM.dma_chx,DMA_MEMORY_0,((uint32_t)memory_dest_addr)+0xffff*size[dma_width]*count);
//        dma_memory_address_config(DMA_MEM.dmax,DMA_MEM.dma_chx,((uint32_t)memory_src_addr));
        dma_periph_address_config(DMA_MEM.dmax,DMA_MEM.dma_chx,((uint32_t)memory_src_addr)+0xffff*size[dma_width]*count);     
        
        count++;        
        if(length>0xffff){            
            dma_transfer_number_config(DMA_MEM.dmax,DMA_MEM.dma_chx,0xffff);
            length-=0xffff;
        }else{
            dma_transfer_number_config(DMA_MEM.dmax,DMA_MEM.dma_chx,length);
            length=0;            
        }        
        dma_channel_enable(DMA_MEM.dmax,DMA_MEM.dma_chx);
        driver_dma_flag_wait_timeout(&DMA_MEM,DMA_FLAG_FTF,SET);
    }while(length);
}

/*!
* 说明     dma内存到内存启动
* 输入[1]  dmax：dmax结构体指针 
* 输入[2]  periph_addr：外设地址 
* 输入[3]  memory_addr：内存地址
* 输入[4]  dma_width：传输宽度
* 输入[4]  dma_direction：传输方向 
* 返回值   无
*/
void driver_dma_mem_to_exmclcd_start(void* memory_dest_addr,void* memory_src_addr,dma_width_enum dma_width,uint32_t length)
{    
    __IO uint32_t count=0;
    const uint8_t size[3]={1,2,4};
//    driver_dma_flag_wait_timeout(&DMA_MEM,DMA_FLAG_FTF,SET);    
    dma_channel_disable(DMA_MEM.dmax,DMA_MEM.dma_chx);
    
    dma_transfer_direction_config(DMA_MEM.dmax,DMA_MEM.dma_chx,DMA_MEMORY_TO_MEMORY);
    
    dma_memory_width_config(DMA_MEM.dmax,DMA_MEM.dma_chx,dma_width_mem[dma_width]);
    dma_periph_width_config(DMA_MEM.dmax,DMA_MEM.dma_chx,dma_width_per[dma_width]);
    
//    dma_memory_increase_enable(DMA_MEM.dmax,DMA_MEM.dma_chx);
//    dma_periph_increase_disable(DMA_MEM.dmax,DMA_MEM.dma_chx);
    
    dma_peripheral_address_generation_config(DMA_MEM.dmax,DMA_MEM.dma_chx,DMA_PERIPH_INCREASE_ENABLE);
    dma_memory_address_generation_config(DMA_MEM.dmax,DMA_MEM.dma_chx,DMA_MEMORY_INCREASE_DISABLE);      
    
    dma_priority_config(DMA_MEM.dmax,DMA_MEM.dma_chx,DMA_PRIORITY_LOW);
    
    dma_circulation_disable(DMA_MEM.dmax, DMA_MEM.dma_chx);
    
//    dma_memory_to_memory_enable(DMA_MEM.dmax, DMA_MEM.dma_chx);    
    
    do{  
        if(count!=0){        
//            driver_dma_flag_wait_timeout(&DMA_MEM,DMA_FLAG_FTF,SET);
            dma_channel_disable(DMA_MEM.dmax,DMA_MEM.dma_chx);        
        }
        
        dma_memory_address_config(DMA_MEM.dmax,DMA_MEM.dma_chx,DMA_MEMORY_0,((uint32_t)memory_dest_addr));
//        dma_memory_address_config(DMA_MEM.dmax,DMA_MEM.dma_chx,((uint32_t)memory_src_addr));
        dma_periph_address_config(DMA_MEM.dmax,DMA_MEM.dma_chx,((uint32_t)memory_src_addr)+0xffff*size[dma_width]*count);     
        
        count++;        
        if(length>0xffff){            
            dma_transfer_number_config(DMA_MEM.dmax,DMA_MEM.dma_chx,0xffff);
            length-=0xffff;
        }else{
            dma_transfer_number_config(DMA_MEM.dmax,DMA_MEM.dma_chx,length);
            length=0;            
        }        
        dma_channel_enable(DMA_MEM.dmax,DMA_MEM.dma_chx);
        driver_dma_flag_wait_timeout(&DMA_MEM,DMA_FLAG_FTF,SET);
    }while(length);
}


/*!
* 说明     dma启动传输
* 输入[1]  dmax：dmax结构体指针 
* 输入[2]  pbuff：内存指针
* 输入[3]  length：长度
* 返回值   无
*/
void driver_dma_start(typdef_dma_general* dmax,uint8_t *pbuff,uint16_t length)
{
    dma_channel_disable(dmax->dmax,dmax->dma_chx);
    dma_memory_address_config(dmax->dmax,dmax->dma_chx,DMA_MEMORY_0,(uint32_t)pbuff);
    dma_transfer_number_config(dmax->dmax,dmax->dma_chx,length);
    dma_flag_clear(dmax->dmax, dmax->dma_chx,DMA_FLAG_FTF);
    dma_channel_enable(dmax->dmax,dmax->dma_chx);
}

/*!
* 说明     dma超时等待
* 输入[1]  dmax：dmax结构体指针 
* 输入[2]  flag:等待标志
* 输入[3]  wait_state：等待状态
* 返回值   无
*/
Drv_Err driver_dma_flag_wait_timeout(typdef_dma_general* dmax, uint32_t flag ,FlagStatus wait_state)
{
    __IO uint64_t timeout = driver_tick;
    if((DMA_CHCTL(dmax->dmax, dmax->dma_chx)&DMA_CHXCTL_CHEN)==DMA_CHXCTL_CHEN)
    {
        while(wait_state!=dma_flag_get(dmax->dmax, dmax->dma_chx,flag)){
            if((timeout+DMA_TIMEOUT_MS) <= driver_tick) {              
                return DRV_ERROR;
            } 
        }
        dma_flag_clear(dmax->dmax, dmax->dma_chx,flag);
    }
    return DRV_SUCCESS;
}

