#ifndef __LED_H_
#define __LED_H_

#include "gd32h7xx.h"

/* 定义LED引脚和端口 */
#define LED1_PIN GPIO_PIN_3
#define LED1_PORT GPIOE

#define LED2_PIN GPIO_PIN_14
#define LED2_PORT GPIOB

#define LED3_PIN GPIO_PIN_3
#define LED3_PORT GPIOG

#define LED4_PIN GPIO_PIN_2
#define LED4_PORT GPIOE

#define LED1_TOGGLE()   do{ gpio_bit_toggle(LED1_PORT, LED1_PIN); }while(0)       /* 翻转LED1 */
#define LED2_TOGGLE()   do{ gpio_bit_toggle(LED2_PORT, LED2_PIN); }while(0)       /* 翻转LED2 */
#define LED3_TOGGLE()   do{ gpio_bit_toggle(LED3_PORT, LED3_PIN); }while(0)       /* 翻转LED3 */
#define LED4_TOGGLE()   do{ gpio_bit_toggle(LED4_PORT, LED4_PIN); }while(0)       /* 翻转LED4 */

#define LED1_ON     led_switch(LED1, led_on) 
#define LED2_ON     led_switch(LED2, led_on) 
#define LED3_ON     led_switch(LED3, led_on) 
#define LED4_ON     led_switch(LED4, led_on) 
#define LED1_OFF    led_switch(LED1, led_off) 
#define LED2_OFF    led_switch(LED2, led_off) 
#define LED3_OFF    led_switch(LED3, led_off) 
#define LED4_OFF    led_switch(LED4, led_off) 

typedef enum {
    LED1 = 0U,
    LED2 = 1U,
    LED3 = 2U,
    LED4 = 3U
} led_target_t;

typedef enum {
    led_off = 0U,
    led_on = 1U
} led_status_t;

void led_gpio_init(void);
void led_switch(led_target_t target_led, led_status_t led_target_status);

#endif /* __LED_H_ */