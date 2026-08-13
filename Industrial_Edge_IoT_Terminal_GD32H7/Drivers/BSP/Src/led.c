#include "led.h"

/* LED GPIO初始化 */
void led_gpio_init(void)
{
    /* 1. 开启GPIO时钟 */
    rcu_periph_clock_enable(RCU_GPIOB); // 用于PB14
    rcu_periph_clock_enable(RCU_GPIOE); // 用于PE2, PE3
    rcu_periph_clock_enable(RCU_GPIOG); // 用于PG3

    /* 2. 配置LED引脚为推挽输出模式 */
    // PB14
    // gpio_mode_set(LED4_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED4_PIN);
    // gpio_output_options_set(LED4_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, LED4_PIN);
    // gpio_bit_reset(LED4_PORT, LED4_PIN); // 初始化为低电平（熄灭）

    // PE2
    gpio_mode_set(LED1_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED1_PIN);
    gpio_output_options_set(LED1_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, LED1_PIN);
    gpio_bit_reset(LED1_PORT, LED1_PIN);

    // PE3
    gpio_mode_set(LED2_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED2_PIN);
    gpio_output_options_set(LED2_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, LED2_PIN);
    gpio_bit_reset(LED2_PORT, LED2_PIN);

    // PG3
    gpio_mode_set(LED3_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED3_PIN);
    gpio_output_options_set(LED3_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, LED3_PIN);
    gpio_bit_reset(LED3_PORT, LED3_PIN);
}

void led_switch(led_target_t target_led, led_status_t led_target_status){
    switch(target_led){
        case LED1:
            if(led_target_status == led_on){
                gpio_bit_set(LED1_PORT, LED1_PIN);
            }else{
                gpio_bit_reset(LED1_PORT, LED1_PIN);
            }
            break;
        case LED2:
            if(led_target_status == led_on){
                gpio_bit_set(LED2_PORT, LED2_PIN);
            }else{
                gpio_bit_reset(LED2_PORT, LED2_PIN);
            }
            break;
        case LED3:
            if(led_target_status == led_on){
                gpio_bit_set(LED3_PORT, LED3_PIN);
            }else {
                gpio_bit_reset(LED3_PORT, LED3_PIN);
            }
            break;
        case LED4:
            if(led_target_status == led_on){
                gpio_bit_set(LED4_PORT, LED4_PIN);
            }else {
                gpio_bit_reset(LED4_PORT, LED4_PIN);
            }
            break;
        default:
            break;
    }
}