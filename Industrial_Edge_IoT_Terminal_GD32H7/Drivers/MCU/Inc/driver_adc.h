/*!
 * 文件名称： driver_adc.h
 * 描    述： adc 驱动文件
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

#ifndef DRIVER_ADC_H
#define DRIVER_ADC_H
#include "public.h"

/*ADC IO口及通道参数*/
typedef struct __typdef_adc_ch_parameter
{
	rcu_periph_enum rcu_port;		//IO口时钟
	uint32_t port;							//IO port
	uint32_t pin;								//IO pin
	uint32_t gpio_speed;				//IO 速率
	uint8_t  adc_channel;				//IO对应的ADC通道
	uint32_t sample_time;				//IO的采样周期
}typdef_adc_ch_parameter;

typedef struct __typdef_adc_dma_parameter
{
	rcu_periph_enum rcu_dma;		//DMA时钟
	uint32_t dma_periph;				//DMA号
	dma_channel_enum dma_channel;//DMA通道号
	uint32_t request;
	uint32_t dma_number;				//DMA传输个数
	uint32_t dma_priority;			//DMA通道优先级
	EventStatus dma_circulation_mode;//循环模式
}typdef_adc_dma_parameter;

typedef struct __typdef_adc_general
{
	rcu_periph_enum rcu_adc;		//ADC时钟口
	uint32_t adc_psc;						//ADC时钟源分频系数
	uint32_t adc_port;					//ADC号
	uint32_t adc_mode;					//ADC工作模式：ADC_MODE_FREE,ADC_DAUL_REGULAL_PARALLEL
	uint8_t adc_channel_group;	//ADC工作组：规则组或注入组
	EventStatus adc_scan_function;//设置扫描模式
	EventStatus adc_continuous_function;//设置循环模式
	uint32_t adc_external_trigger_mode;
	uint8_t ch_count;						//设置转换通道个数
	typdef_adc_dma_parameter dma_parameter;//若使用DMA,则需要设置dma
	uint32_t trigger_source;			//ADC触发源
	EventStatus DMA_mode;					//是否使用DMA
}typdef_adc_ch_general;

void driver_adc_config(typdef_adc_ch_general *ADC,typdef_adc_ch_parameter *ADC_CH);
void driver_adc_inserted_ch_config(typdef_adc_ch_general *ADC,typdef_adc_ch_parameter *ADC_CH);
void driver_adc_software_trigger_enable(typdef_adc_ch_general *ADC);
void driver_adc_regular_ch_dma_config(typdef_adc_ch_general *ADC, typdef_adc_ch_parameter *ADC_CH,void *buffer);
uint16_t driver_adc_transform_polling(typdef_adc_ch_general *ADC,typdef_adc_ch_parameter *ADC_CH);
void driver_adc_int_handler(typdef_adc_ch_general *ADC,void *buffer);
void driver_adc_dma_int_handler(typdef_adc_ch_general *ADC);
void ADC_int_enable(typdef_adc_ch_general *ADC);
void ADC_dma_int_enable(typdef_adc_ch_general *ADC);
#endif /* DRIVER_ADC_H*/
