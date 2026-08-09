/*!
 * 文件名称： driver_gpio.c
 * 描    述： gpio 驱动文件
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

#include "driver_gpio.h"

/*!  GPIO定义注册示例

GPIO_DEF(LED2,E,5,OUT_PP,SET,NULL);              // PE5定义为LED2,OUTPP模式，默认电平高，无回调函数

GPIO_DEF(KEY0,E,2,INT_LOW, SET,KEY0_IRQHandler); // PE2定义为KEY0,低电平触发中断模式，默认电平高，中断回调执行KEY0_IRQHandler函数

GPIO_DEF(USER_KEY,E,3,IN_PU,SET,NULL);               // PE3定义为USER_KEY,上拉输入模式，默认电平高

AFIO_DEF(UART0_TX,B,6,AF_PP,GPIO_USART0_REMAP);  //定义PB6引脚为UART0_TX，复用推挽模式，配置重映射参数为GPIO_USART0_REMAP

*/

/*!
* 说明     gpio初始化
* 输入[1]  gpio注册参数
* 返回值   无
*/
void driver_gpio_general_init(typdef_gpio_general *gpio)
{
    static const uint8_t MODE_TABLE[]={GPIO_MODE_OUTPUT,GPIO_MODE_OUTPUT,GPIO_MODE_INPUT,GPIO_MODE_INPUT,GPIO_MODE_INPUT,GPIO_MODE_INPUT,GPIO_MODE_INPUT,GPIO_MODE_AF,GPIO_MODE_AF};
    static const uint8_t PULL_MODE[]={GPIO_PUPD_NONE,GPIO_PUPD_PULLUP,GPIO_PUPD_NONE,GPIO_PUPD_PULLUP,GPIO_PUPD_PULLDOWN,GPIO_PUPD_PULLUP,GPIO_PUPD_PULLDOWN,GPIO_PUPD_NONE,GPIO_PUPD_PULLUP};
    static const uint8_t OUT_TYPE[]={GPIO_OTYPE_PP,GPIO_OTYPE_OD,GPIO_OTYPE_OD,GPIO_OTYPE_OD,GPIO_OTYPE_OD,GPIO_OTYPE_OD,GPIO_OTYPE_OD,GPIO_OTYPE_PP,GPIO_OTYPE_OD};

    
    if(gpio!=NULL){
        /* 打开GPIO时钟 */
        rcu_periph_clock_enable(gpio->rcu_port);
        rcu_periph_clock_enable(RCU_SYSCFG);
        
        /* 复用模式预设 */
        if (gpio->gpio_mode == AF_PP || gpio->gpio_mode == AF_OD)
        {
            gpio_af_set(gpio->port,gpio->afio_mode,gpio->pin);
        }    
            
        /* 预设IO输出电平 */
        if (gpio->gpio_mode == OUT_PP || gpio->gpio_mode == OUT_OD)
        {
            gpio_bit_write(gpio->port,gpio->pin,gpio->default_state);
        }
        
        /* 初始化GPIO模式 */
        gpio_mode_set(gpio->port,MODE_TABLE[gpio->gpio_mode],PULL_MODE[gpio->gpio_mode],gpio->pin);    
        /* 初始化GPIO输出模式 */
        gpio_output_options_set(gpio->port,OUT_TYPE[gpio->gpio_mode],gpio->speed,gpio->pin);       
      
        if (gpio->gpio_mode == INT_HIGH || gpio->gpio_mode == INT_LOW)
        {
            //* EXTI源配置 */
//            gpio_exti_source_select(gpio->int_port,gpio->int_pin);            
            syscfg_exti_line_config(gpio->int_port,gpio->int_pin);
            
            /* 对应EXTI线模式配置 */
            if(gpio->gpio_mode==INT_HIGH){
                exti_init(gpio->extix, EXTI_INTERRUPT, EXTI_TRIG_RISING);
            }else{
                exti_init(gpio->extix, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
            }            
            exti_interrupt_flag_clear(gpio->extix);
        }
    }
}

/*!
* 说明     gpio输出值设置
* 输入[1]  gpio注册参数
* 输入[2]  gpio输出值
* 返回值   无
*/
void driver_gpio_pin_write(  typdef_gpio_general *gpio, bit_status bit_vaule)
{
    gpio_bit_write(gpio->port,gpio->pin,bit_vaule);
}

/*!
* 说明     gpio输出高
* 输入[1]  gpio注册参数
* 返回值   无
*/
void driver_gpio_pin_set(  typdef_gpio_general *gpio)
{
    gpio_bit_set(gpio->port, gpio->pin);
}

/*!
* 说明     gpio输出低
* 输入[1]  gpio注册参数
* 返回值   无
*/
void driver_gpio_pin_reset(  typdef_gpio_general *gpio)
{
    gpio_bit_reset(gpio->port, gpio->pin);
}

/*!
* 说明     gpio输出翻转
* 输入[1]  gpio注册参数
* 返回值   无
*/
void driver_gpio_pin_toggle(  typdef_gpio_general *gpio)
{
    gpio_bit_write(gpio->port,gpio->pin,(bit_status)(gpio_input_bit_get(gpio->port, gpio->pin)^1));
}

/*!
* 说明     gpio输入获取
* 输入[1]  gpio注册参数
* 返回值   无
*/
bit_status driver_gpio_pin_get( typdef_gpio_general *gpio)
{
    return (bit_status)gpio_input_bit_get(gpio->port, gpio->pin);
}

/*!
* 说明     gpio输入滤波读取
* 输入[1]  gpio注册参数
* 返回值   无
*/
bit_status dvire_gpio_pin_filter_get(typdef_gpio_general *gpio,uint8_t filter_num)
{
        uint8_t bit_num=0;

        for(uint8_t i=0;i<filter_num;i++)
        {
            delay_sysclk(20);
            bit_num+=driver_gpio_pin_get(gpio);
        }
        if(bit_num<filter_num/2)
            return RESET;
        else
            return SET;
}

/*!
* 说明     gpio输入中断模式中断服务函数处理
* 输入[1]  gpio注册参数
* 返回值   无
*/
void dvire_gpio_exti_handle(typdef_gpio_general *gpio)
{
    bit_status int_input_bit=RESET;
    if(exti_flag_get(gpio->extix)==SET)
    {
        exti_flag_clear(gpio->extix);
        //int_input_bit=dvire_gpio_pin_filter_get(gpio,10);

        if( (gpio->gpio_mode==INT_LOW && int_input_bit==RESET) || (gpio->gpio_mode==INT_HIGH && int_input_bit==SET) )
        {
            if(gpio->int_callback!=NULL)
            {                
                gpio->int_callback((typdef_gpio_general *)gpio);                
            }    
        }   
    }
}

