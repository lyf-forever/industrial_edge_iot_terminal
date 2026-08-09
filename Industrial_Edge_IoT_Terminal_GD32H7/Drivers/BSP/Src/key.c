#include "key.h"
#include "gd32h7xx_gpio.h"
#include "public.h"


/**
 * @brief       按键初始化函数
 * @param       无
 * @retval      无
 */
void key_init(void)
{
    rcu_periph_clock_enable(KEY0_GPIO_CLK);     /* 使能KEY0时钟 */
    rcu_periph_clock_enable(WKUP_GPIO_CLK);     /* 使能WKUP时钟 */
    
    gpio_mode_set(KEY0_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, KEY0_GPIO_PIN);  /* KEY0引脚模式设置,下拉输入 */
    
    gpio_mode_set(WKUP_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, WKUP_GPIO_PIN);  /* WKUP引脚模式设置,下拉输入 */  	
}

/**
 * @brief       按键扫描函数
 * @note        该函数有响应优先级(同时按下多个按键): WK_UP > KEY0!!
 * @param       mode:0 / 1, 具体含义如下:
 *   @arg       0,  不支持连续按(当按键按下不放时, 只有第一次调用会返回键值,
 *                  必须松开以后, 再次按下才会返回其他键值)
 *   @arg       1,  支持连续按(当按键按下不放时, 每次调用该函数都会返回键值)
 * @retval      键值, 定义如下:
 *              KEY0_PRES, 1, KEY0按下
 *              WKUP_PRES, 2, WKUP按下
 */
uint8_t key_scan(uint8_t mode)
{
    static uint8_t key_up = 1;  /* 按键按松开标志 */
    uint8_t keyval = 0;

    if (mode) key_up = 1;       /* 支持连按 */
  
    if (key_up && (KEY0 == 0 || WK_UP == 1))     /* 按键松开标志为1, 且有任意一个按键按下了 */
    {
        delay_ms(10);           /* 去抖动 */
        key_up = 0;

        if (KEY0 == 0)  keyval = KEY0_PRES;
      
        if (WK_UP == 1) keyval = WKUP_PRES;
    }
    else if (KEY0 == 1 && WK_UP == 0)            /* 没有任何按键按下, 标记按键松开 */
    {
        key_up = 1;
    }

    return keyval;              /* 返回键值 */
}


