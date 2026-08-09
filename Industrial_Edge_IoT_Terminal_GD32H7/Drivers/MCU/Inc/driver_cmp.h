/*!
 * 文件名称： driver_cmp.h
 * 描    述： cmp 驱动文件
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

#ifndef DRIVER_CMP_H
#define DRIVER_CMP_H
#include "public.h"

typedef struct __typdef_cmp_struct
{
	rcu_periph_enum rcu_cmpx;						//CMP时钟口
	uint32_t cmp_x; 										//比较器外设
	uint32_t operating_mode;						//比较器操作模式
	uint32_t inverting_input;						//反向输入选择
	uint32_t output_hysteresis;					//输出迟滞配置
	uint32_t VREF_scaler_en;						//参考电压标定器使能控制：ENABLE/DISABLE
	uint32_t noninverting_input;				//正向输入配置
	uint32_t output_polarity;						//输出极性配置
	uint32_t int_en;                    //中断使能控制
	
  typdef_gpio_general* cmp_forword_input_gpio;  //正向输入引脚结构体
  typdef_gpio_general* cmp_reverse_input_gpio;  //反向输入引脚结构体
  typdef_gpio_general* cmp_output_gpio;					//输出引脚结构体
  typdef_gpio_general* cmp_MUX_output_gpio;			//MUX输出引脚结构体
	
  void (*cmp_int_callback)(struct __typdef_cmp_struct *cmpx);  //比较器中断回调函数
	
}typdef_cmp_struct;
   				
void driver_cmp_init(typdef_cmp_struct *cmpx);
void driver_cmp_int_handler(typdef_cmp_struct *cmpx);
#endif /* DRIVER_CMP_H*/
