/*!
 * 文件名称： driver_cmp.c
 * 描    述： cmp 驱动文件
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
#include "driver_cmp.h"

/*!
* 说明     CMP初始化
* 输入[1]  cmpx:   CMP配置结构体指针
* 返回值   无
*/
void driver_cmp_init(typdef_cmp_struct *cmpx)
{	 
	  
	  rcu_periph_clock_enable(cmpx->rcu_cmpx);
		/* cmp  configure */
		usart_deinit(cmpx->cmp_x);

	  /* 配置正向输入引脚*/
	  if(cmpx->cmp_forword_input_gpio!=NULL)
		{
				rcu_periph_clock_enable(cmpx->cmp_forword_input_gpio->rcu_port);
			  gpio_mode_set(cmpx->cmp_forword_input_gpio->port,GPIO_MODE_ANALOG,GPIO_PUPD_NONE,cmpx->cmp_forword_input_gpio->pin);
		}
		/* 配置反向输入引脚*/
	  if(cmpx->cmp_reverse_input_gpio!=NULL)
		{
				rcu_periph_clock_enable(cmpx->cmp_forword_input_gpio->rcu_port);
			  gpio_mode_set(cmpx->cmp_forword_input_gpio->port,GPIO_MODE_ANALOG,GPIO_PUPD_NONE,cmpx->cmp_forword_input_gpio->pin);
		}
		/* 配置输出引脚*/
	  if(cmpx->cmp_output_gpio!=NULL)
		{
				driver_gpio_general_init(cmpx->cmp_output_gpio);
		}
		/* 配置MUX输出引脚*/
		if(cmpx->cmp_MUX_output_gpio!=NULL)
		{
				driver_gpio_general_init(cmpx->cmp_MUX_output_gpio);
		}
		
		
	  /* 去初始化 CMPX */
    cmp_deinit(cmpx->cmp_x);
    /* 使能VREF电压标定器 */
		if(cmpx->VREF_scaler_en == ENABLE)
		{
				cmp_voltage_scaler_enable(CMP0);
				cmp_scaler_bridge_enable(CMP0);
		}

    /* 配置CMPx正向输入 */
		if(cmpx->noninverting_input!=0)
		{
				cmp_noninverting_input_select(cmpx->cmp_x, cmpx->noninverting_input);
		}
		/* 配置CMPx */
    cmp_mode_init(cmpx->cmp_x, cmpx->operating_mode, cmpx->inverting_input, cmpx->output_hysteresis);
    /* 配置CMPx输出 */
		if(cmpx->output_polarity != 0)
		{
        cmp_output_init(cmpx->cmp_x, cmpx->output_polarity );
		}
		/* 配置CMPx中断 */
		if(cmpx->int_en == ENABLE)
		{
			if(cmpx->cmp_x == CMP0)
			{
					/* 配置CMP0中断，对应EXTI20 */
					exti_flag_clear(EXTI_20);
					exti_init(EXTI_20, EXTI_INTERRUPT, EXTI_TRIG_BOTH);
					exti_interrupt_enable(EXTI_20);
			}else{
					/* 配置CMP1中断，对应EXTI21 */
					exti_flag_clear(EXTI_21);
					exti_init(EXTI_21, EXTI_INTERRUPT, EXTI_TRIG_BOTH);
					exti_interrupt_enable(EXTI_21);
			}
			
					/* 配置 CMP NVIC */
					nvic_irq_enable(CMP0_1_IRQn, 0, 0);
		}

    /* 使能 CMPX */
    cmp_enable(cmpx->cmp_x);
}

/*!
* 说明     CMP中断处理驱动函数
* 输入[1]  cmpx:   CMP配置结构体指针
* 返回值   无
*/
void driver_cmp_int_handler(typdef_cmp_struct *cmpx)
{
		if(cmpx->cmp_x== CMP0)
		{
				if(RESET != exti_interrupt_flag_get(EXTI_20)) {
						exti_interrupt_flag_clear(EXTI_20);
					
						if(cmpx->cmp_int_callback != NULL)
						{
								cmpx->cmp_int_callback(cmpx);
						}
				}
		}
		if(cmpx->cmp_x==CMP1)
		{
				if(RESET != exti_interrupt_flag_get(EXTI_21)) {
						exti_interrupt_flag_clear(EXTI_21);
					
						if(cmpx->cmp_int_callback != NULL)
						{
								cmpx->cmp_int_callback(cmpx);
						}
				}
	  }
}
    
