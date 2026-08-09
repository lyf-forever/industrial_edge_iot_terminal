/*!
 * 文件名称： driver_gpio.h
 * 描    述： gpio 驱动文件
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

#ifndef DRIVER_GPIO_H
#define DRIVER_GPIO_H
#include "public.h"

typedef enum
{
    OUT_PP=0,
    OUT_OD,
    IN_NONE,    
    IN_PU,
    IN_PD,
    INT_HIGH,
    INT_LOW,
    AF_PP,
    AF_OD,   
}gpio_mode_enum;    

typedef struct __typdef_gpio_general
{
    rcu_periph_enum rcu_port;
    uint32_t port;
    uint32_t pin;
    gpio_mode_enum gpio_mode;
    uint32_t speed;
    bit_status default_state;
    uint32_t int_port;
    uint32_t int_pin;
    exti_line_enum extix;
    void (*int_callback)(struct __typdef_gpio_general *exti);
    uint32_t afio_mode;
}typdef_gpio_general;

#define GPIO_DEF(name,port,pin,mode,default_state,int_callback)       \
        typdef_gpio_general name = \
        {RCU_GPIO##port,GPIO##port,GPIO_PIN_##pin,mode,GPIO_OSPEED_12MHZ,default_state,EXTI_SOURCE_GPIO##port,EXTI_SOURCE_PIN##pin,EXTI_##pin,int_callback,0}

#define AFIO_DEF(name,port,pin,mode,afio_mode)       \
        typdef_gpio_general name = \
        {RCU_GPIO##port,GPIO##port,GPIO_PIN_##pin,mode,GPIO_OSPEED_12MHZ,SET,EXTI_SOURCE_GPIO##port,EXTI_SOURCE_PIN##pin,EXTI_##pin,NULL,afio_mode}        
        
#define GPIO_DEF_EXTERN(name) \
    extern typdef_gpio_general name
 
void driver_gpio_general_init(typdef_gpio_general *gpio);
void driver_gpio_pin_write(typdef_gpio_general *gpio, bit_status bit_vaule);
void driver_gpio_pin_set(typdef_gpio_general *gpio);
void driver_gpio_pin_reset(typdef_gpio_general *gpio);
void driver_gpio_pin_toggle(typdef_gpio_general *gpio);
bit_status driver_gpio_pin_get(typdef_gpio_general *gpio) ;
void dvire_gpio_exti_handle(typdef_gpio_general *gpio);  
bit_status dvire_gpio_pin_filter_get(typdef_gpio_general *gpio,uint8_t filter_num);
#endif /* DRIVER_GPIO_H*/
