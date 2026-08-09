#ifndef __BSP_WS2812_H
#define __BSP_WS2812_H

/*==============================================*
 *      包含的头文件
 *----------------------------------------------*/
#include <stdint.h>
#include "soc/gpio_num.h"
#include "sys.h"

#if Ws2812Use

/*==============================================*
 *       宏定义/常量/typedef/enum
 *----------------------------------------------*/
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      GPIO_NUM_48

#define EXAMPLE_LED_NUMBERS         1

#define EXAMPLE_FRAME_DURATION_MS   2000      //彩虹灯条刷新时间，当前未使用
#define EXAMPLE_ANGLE_INC_FRAME     1       //越大则每次调用rainbow时LED颜色变化越明显
#define EXAMPLE_ANGLE_INC_LED       0.3     //越大则调用rainbow时整条灯带每个灯珠的颜色差异越明显
/*==============================================*
 *      全局变量
 *----------------------------------------------*/

/*==============================================*
 *      函数实现
 *----------------------------------------------*/
void ws2812_init(void);
void ws2812_writeRainbow(void);
void ws2812_writeGRB(uint8_t g, uint8_t r, uint8_t b);
void ws2812_on(void);
void ws2812_off(void);

#endif /* Ws2812Use */

#endif 

