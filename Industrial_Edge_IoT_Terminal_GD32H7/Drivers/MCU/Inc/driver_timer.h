/*!
 * 文件名称： driver_timer.h
 * 描    述： timer 驱动文件
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

#ifndef DRIVER_TIMER_H
#define DRIVER_TIMER_H
#include "public.h"

typedef enum 
{
    TIMER_CH_INPUT_CAPTURE_LOW=0,
    TIMER_CH_INPUT_CAPTURE_HIGH,
    TIMER_CH_INPUT_CAPTURE_PWM,    
    TIMER_CH_PWM_HIGH,    
    TIMER_CH_PWM_LOW,    
}timer_ch_enum;

typedef struct __typdef_timer_ch_struct
{ 
    uint32_t timer_x;
    rcu_periph_enum rcu_timerx;    
    uint16_t ch_x;
    timer_ch_enum timer_ch_mode;
    uint32_t TIMER_INT_CHx;
    uint32_t TIMER_FLAG_CHx;
    typdef_gpio_general* timer_ch_gpio;
    void (*timer_ch_callback)(struct __typdef_timer_ch_struct* timer_chx);  
    
}typdef_timer_ch_struct; 

typedef struct __typdef_timusimer_struct
{
    uint32_t timer_x; 
    rcu_periph_enum rcu_timerx;
    void (*timer_updata_callback)(void);           
}typdef_timer_base_struct;
                 
                        
#define TIMER_CH_DEF(name, timer_x,ch_x,timer_ch_mode,port,pin,gpio_mode,afio_mode)       \
        AFIO_DEF(name##_TIMER_GPIO,port,pin,gpio_mode,afio_mode);\
        typdef_timer_ch_struct name = \
        {timer_x,RCU_##timer_x,TIMER_CH_##ch_x,timer_ch_mode,TIMER_INT_CH##ch_x,TIMER_FLAG_CH##ch_x,&name##_TIMER_GPIO,NULL}        
        
        
#define TIMER_BASE_DEF(name,timer_x)      \
        typdef_timer_base_struct name = \
        {timer_x,RCU_##timer_x,NULL} 

#define TIMER_CH_DEF_EXTERN(name) \
    extern typdef_timer_ch_struct name 

#define TIMER_BASE_DEF_EXTERN(name) \
    extern typdef_timer_base_struct name     

void driver_timer_base_int_init(typdef_timer_base_struct* timerx,uint32_t counter_frequency,uint16_t period);
void driver_timer_channel_init(typdef_timer_ch_struct* timerx_ch,uint32_t counter_frequency,uint16_t period);
void driver_timer_multi_channel_init(typdef_timer_ch_struct* timerx_cha,typdef_timer_ch_struct* timerx_chb,typdef_timer_ch_struct* timerx_chc,typdef_timer_ch_struct* timerx_chd,uint32_t counter_frequency,uint16_t period);    
void driver_timer_pwm_duty_set(typdef_timer_ch_struct* timerx_ch,uint16_t duty_value);
void driver_timer_pwm_period_set(typdef_timer_ch_struct* timerx_ch,uint16_t period);
void driver_timer_pwm_period_duty_set(typdef_timer_ch_struct* timerx_ch,uint16_t period,uint16_t duty_value);
void driver_timer_pwm_on(typdef_timer_ch_struct* timerx_ch);        
void driver_timer_pwm_off(typdef_timer_ch_struct* timerx_ch);
void driver_timer_updata_int_handler(typdef_timer_base_struct* timerx);
void driver_timer_channel_int_handler(typdef_timer_ch_struct* timerx_ch);    
#endif /* DRIVER_TIMER_H*/
