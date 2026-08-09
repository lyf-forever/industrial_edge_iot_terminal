/*!
 * 文件名称： driver_adc.c
 * 描    述： adc 驱动文件
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

#include "public.h"
#include "driver_adc.h"

/*!
* 说明     ADC初始化，包括ADC时钟使能和功能配置
* 输入[1]  ADC：   ADC配置结构体指针
* 输入[2]  ADC_CH：ADC通道配置结构体指针
* 返回值   无
*/
void driver_adc_config(typdef_adc_ch_general *ADC,typdef_adc_ch_parameter *ADC_CH)
{	 
	uint8_t i;
    adc_idx_enum idx_adc;
	adc_deinit(ADC->adc_port);
    /* ADC clock config */
    
    if(ADC->adc_port==ADC0){
        idx_adc=IDX_ADC0;    
    }else if(ADC->adc_port==ADC1){
        idx_adc=IDX_ADC1;    
    }else{
        idx_adc=IDX_ADC2;    
    }   
    rcu_adc_clock_config(idx_adc, RCU_ADCSRC_PER);    
    
	adc_clock_config(ADC->adc_port, ADC->adc_psc); 					/*配置ADC时钟频率*/
	rcu_periph_clock_enable(ADC->rcu_adc); 				/*使能ADC时钟*/
    
//    /* ADC clock config */
//    rcu_adc_clock_config(IDX_ADC2, RCU_ADCSRC_PER);
//    adc_clock_config(ADC2, ADC_CLK_ASYNC_DIV64);    
    
	/*配置ADC相关IO口，先配置时钟，再将IO口设置为模拟输入*/
	for(i=0 ;i<ADC->ch_count; i++)
	{
		if(ADC_CH[i].adc_channel < ADC_CHANNEL_17)
		{
			rcu_periph_clock_enable(ADC_CH[i].rcu_port);
			gpio_mode_set(ADC_CH[i].port, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ADC_CH[i].pin);					
		}
		else
		{
			if(ADC->adc_port==ADC2)
			{
				if(ADC_CH[i].adc_channel == ADC_CHANNEL_17)
				{
					adc_internal_channel_config(ADC_CHANNEL_INTERNAL_VBAT, ENABLE);
				}
				else if(ADC_CH[i].adc_channel == ADC_CHANNEL_18)
				{
					adc_internal_channel_config(ADC_CHANNEL_INTERNAL_TEMPSENSOR, ENABLE);
				}
				else if(ADC_CH[i].adc_channel == ADC_CHANNEL_19)
				{
					adc_internal_channel_config(ADC_CHANNEL_INTERNAL_VREFINT, ENABLE);
				}
				else if(ADC_CH[i].adc_channel == ADC_CHANNEL_20)
				{
					adc_internal_channel_config(ADC_CHANNEL_INTERNAL_HP_TEMPSENSOR, ENABLE);
				}				
			}
			else
			{
				rcu_periph_clock_enable(ADC_CH[i].rcu_port);
				gpio_mode_set(ADC_CH[i].port, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ADC_CH[i].pin);					
			}
		}		

	}
	adc_sync_mode_config(ADC->adc_mode);								/*配置ADC工作模式，如独立模式，规则并行模式等*/
	adc_special_function_config(ADC->adc_port, ADC_SCAN_MODE, ADC->adc_scan_function);	/*配置规则组的扫描模式和连续转换模式*/
	if(ADC->adc_channel_group == ADC_REGULAR_CHANNEL)
	{
		adc_special_function_config(ADC->adc_port, ADC_CONTINUOUS_MODE, ADC->adc_continuous_function);	
	}
	adc_data_alignment_config(ADC->adc_port, ADC_DATAALIGN_RIGHT);											/*选择数据右对齐*/
	adc_channel_length_config(ADC->adc_port, ADC->adc_channel_group, ADC->ch_count);		/*配置转换通道数*/
	if(ADC->adc_channel_group == ADC_REGULAR_CHANNEL)																		/*配置转换顺序*/
	{
		for(i = 0;i< ADC->ch_count;i++)
		{
			adc_regular_channel_config(ADC->adc_port, i, ADC_CH[i].adc_channel,ADC_CH[i].sample_time);
		}
	}
	else if(ADC->adc_channel_group == ADC_INSERTED_CHANNEL)
	{
		for(i = 0;i< ADC->ch_count;i++)
		{
			adc_inserted_channel_config(ADC->adc_port, i, ADC_CH[i].adc_channel,ADC_CH[i].sample_time);
		}	
	}
	/*选择触发源及使能外部触发模式*/
	adc_external_trigger_config(ADC->adc_port, ADC->adc_channel_group, ADC->adc_external_trigger_mode);
	/*选择是否需要使用DMA*/
	if(ADC->DMA_mode == ENABLE)
	{
		adc_dma_request_after_last_enable(ADC->adc_port);
		adc_dma_mode_enable(ADC->adc_port);
	}
	 /*ADC的使能和自校准，ADC使能后需要经过一定的ADC_CLK后才能校准，本示例中直接使用1ms延时*/
	adc_enable(ADC->adc_port);
	delay_ms(1);
	/* ADC calibration mode config */
	adc_calibration_mode_config(ADC->adc_port, ADC_CALIBRATION_OFFSET_MISMATCH);
	/* ADC calibration number config */
	adc_calibration_number(ADC->adc_port, ADC_CALIBRATION_NUM32);
	adc_calibration_enable(ADC->adc_port);
}


/*!
* 说明     软件触发ADC转换函数
* 输入     ADC：ADC配置结构体指针
* 返回值   无
*/
void driver_adc_software_trigger_enable(typdef_adc_ch_general *ADC)
{
    adc_software_trigger_enable(ADC->adc_port, ADC->adc_channel_group);
}


/*!
* 说明     带DMA功能的ADC初始化，包括ADC时钟使能和功能配置
* 输入[1]  ADC：   ADC配置结构体指针
* 输入[2]  ADC_CH：ADC通道配置结构体指针
* 输入[3]  buffer：ADC数据缓存buffer
* 返回值   无
*/
void driver_adc_regular_ch_dma_config(typdef_adc_ch_general *ADC, typdef_adc_ch_parameter *ADC_CH,void *buffer)
{	
	dma_single_data_parameter_struct dma_single_data_parameter;
	rcu_periph_clock_enable(ADC->dma_parameter.rcu_dma);													/*DMA时钟开启*/
	rcu_periph_clock_enable(RCU_DMAMUX);	
	dma_deinit(ADC->dma_parameter.dma_periph, ADC->dma_parameter.dma_channel);		/*DMA通道参数复位*/
	/*DMA源地址、目标地址、增量方式、传输位宽、传输方向、传输个数、优先级设置*/
	dma_single_data_parameter.request = ADC->dma_parameter.request;
	dma_single_data_parameter.periph_addr  = (uint32_t)(&ADC_RDATA(ADC->adc_port));
	dma_single_data_parameter.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;
	dma_single_data_parameter.memory0_addr  = (uint32_t)(buffer);
	dma_single_data_parameter.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;
	if(ADC->adc_mode == ADC_DAUL_REGULAL_PARALLEL)
	{
		dma_single_data_parameter.periph_memory_width = DMA_PERIPH_WIDTH_32BIT;
	}
	else
	{
		dma_single_data_parameter.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
	}
	dma_single_data_parameter.direction    = DMA_PERIPH_TO_MEMORY;
	dma_single_data_parameter.number       = ADC->dma_parameter.dma_number;
	dma_single_data_parameter.priority     = ADC->dma_parameter.dma_priority;
	dma_single_data_mode_init(ADC->dma_parameter.dma_periph, ADC->dma_parameter.dma_channel, &dma_single_data_parameter);
	/*DMA循环模式设置*/
	if(ADC->dma_parameter.dma_circulation_mode == ENABLE)
	{
		dma_circulation_enable(ADC->dma_parameter.dma_periph, ADC->dma_parameter.dma_channel);
	}
	else
	{
		dma_circulation_disable(ADC->dma_parameter.dma_periph, ADC->dma_parameter.dma_channel);
	}
	dma_channel_enable(ADC->dma_parameter.dma_periph, ADC->dma_parameter.dma_channel);	/* 使能DMA */
	driver_adc_config(ADC,ADC_CH);																											/* ADC初始化 */
}


/*!
* 说明     轮训方式获取ADC值
* 输入[1]  ADC：   ADC配置结构体指针
* 输入[2]  ADC_CH：ADC通道配置结构体指针
* 返回值   无
*/
uint16_t driver_adc_transform_polling(typdef_adc_ch_general *ADC,typdef_adc_ch_parameter *ADC_CH)
{
	/*规则组采样*/
	if(ADC->adc_channel_group == ADC_REGULAR_CHANNEL)
	{
		adc_regular_channel_config(ADC->adc_port, 0, ADC_CH->adc_channel, ADC_CH->sample_time);			/*设置规则组需要采样的通道*/
		adc_software_trigger_enable(ADC->adc_port, ADC_REGULAR_CHANNEL);														/*软件触发规则组转换*/
		while(RESET == adc_flag_get(ADC->adc_port,ADC_FLAG_EOC));																		/*等待EOC置起*/
		adc_flag_clear(ADC->adc_port,ADC_FLAG_EOC);																									/*清除EOC标志位*/
		return ADC_RDATA(ADC->adc_port);																														/*将规则组转换结果作为返回值*/
		
	}
	/*注入组采样*/
	else if(ADC->adc_channel_group == ADC_INSERTED_CHANNEL)
	{
		adc_inserted_channel_config(ADC->adc_port, 0, ADC_CH->adc_channel, ADC_CH->sample_time);		/*设置注入组需要采样的通道*/
		adc_software_trigger_enable(ADC->adc_port, ADC_INSERTED_CHANNEL);														/*软件触发注入组转换*/
		while(RESET == adc_flag_get(ADC->adc_port,ADC_FLAG_EOIC));																	/*等待EOIC置起 */
		adc_flag_clear(ADC->adc_port,ADC_FLAG_EOIC);																								/*清除EOIC标志位*/
		return ADC_IDATA0(ADC->adc_port);																														/*将注入组转换结果作为返回值*/
	}
	return 0;
}

/*!
* 说明     ADC中断处理函数
* 输入[1]  ADC：   ADC配置结构体指针
* 输入[2]  buffer：ADC数据缓存buffer
* 返回值   无
*/

void driver_adc_int_handler(typdef_adc_ch_general *ADC,void *buffer)
{
	uint8_t i;
  uint32_t buff_addres=(uint32_t)(buffer);
	if(ADC->adc_channel_group == ADC_REGULAR_CHANNEL)
	{
		if(SET == adc_interrupt_flag_get(ADC->adc_port,ADC_INT_FLAG_EOC))
		{
			adc_interrupt_flag_clear(ADC->adc_port,ADC_INT_FLAG_EOC);
			if(ADC->adc_mode == ADC_DAUL_REGULAL_PARALLEL)
			{
				REG32(buffer) = (uint32_t)(ADC_RDATA(ADC->adc_port));
			}
			else
			{
				REG16(buffer) = (uint16_t)(ADC_RDATA(ADC->adc_port));
			}
			
		}
	}
	else if(ADC->adc_channel_group == ADC_INSERTED_CHANNEL)
	{
		if(SET == adc_interrupt_flag_get(ADC->adc_port,ADC_INT_FLAG_EOIC))
		{
			adc_interrupt_flag_clear(ADC->adc_port,ADC_INT_FLAG_EOIC);
			if(ADC->adc_mode == ADC_DAUL_INSERTED_PARALLEL)
			{
				for(i = 0; i<ADC->ch_count ; i++)
				{
					REG32(buff_addres) = REG32((ADC->adc_port) + 0x3C+(i*4));
					buff_addres += 4;
				}
			}
			else
			{
				for(i = 0; i<ADC->ch_count ; i++)
				{
					REG16(buff_addres) = REG32((ADC->adc_port) + 0x3C+(i*4));
					buff_addres += 2;
				}
			}
		}
	}
}

/*!
* 说明     ADC DMA中断处理函数
* 输入[1]  ADC：ADC配置结构体指针
* 返回值   无
*/
void driver_adc_dma_int_handler(typdef_adc_ch_general *ADC)
{
	if(SET == dma_interrupt_flag_get(ADC->dma_parameter.dma_periph,ADC->dma_parameter.dma_channel,DMA_INT_FLAG_FTF))
	{
		dma_interrupt_flag_clear(ADC->dma_parameter.dma_periph,ADC->dma_parameter.dma_channel,DMA_INT_FLAG_FTF);
	}
}


/*!
* 说明     ADC中断使能函数
* 输入[1]  ADC：   ADC配置结构体指针
* 返回值   无
*/
void ADC_int_enable(typdef_adc_ch_general *ADC)
{
	/*规则组中断使能*/
	if(ADC->adc_channel_group == ADC_REGULAR_CHANNEL)
	{
		adc_interrupt_enable(ADC->adc_port,ADC_INT_EOC);
		adc_interrupt_flag_clear(ADC->adc_port,ADC_INT_EOC);
	}
	/*注入组中断使能*/
	else if(ADC->adc_channel_group == ADC_INSERTED_CHANNEL)
	{
		adc_interrupt_enable(ADC->adc_port,ADC_INT_EOIC);
		adc_interrupt_flag_clear(ADC->adc_port,ADC_INT_EOIC);
	}	
}


/*!
* 说明     ADC DMA使能函数
* 输入[1]  ADC：   ADC配置结构体指针
* 返回值   无
*/
void ADC_dma_int_enable(typdef_adc_ch_general *ADC)
{
	dma_interrupt_enable(ADC->dma_parameter.dma_periph,ADC->dma_parameter.dma_channel,DMA_CHXCTL_FTFIE);	
}

