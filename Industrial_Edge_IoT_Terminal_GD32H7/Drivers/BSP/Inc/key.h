#ifndef __KEY_H
#define __KEY_H

#include "sys.h"


/******************************************************************************************/
/* 引脚 定义 */

#define KEY0_GPIO_PORT                  GPIOE
#define KEY0_GPIO_PIN                   GPIO_PIN_4
#define KEY0_GPIO_CLK                   RCU_GPIOE     /* GPIOE时钟使能 */

#define WKUP_GPIO_PORT                  GPIOC
#define WKUP_GPIO_PIN                   GPIO_PIN_13
#define WKUP_GPIO_CLK                   RCU_GPIOC     /* GPIOC时钟使能 */

#define KEY0        gpio_input_bit_get(KEY0_GPIO_PORT, KEY0_GPIO_PIN)   /* 读取KEY0引脚 */
#define WK_UP       gpio_input_bit_get(WKUP_GPIO_PORT, WKUP_GPIO_PIN)   /* 读取WKUP引脚 */

#define KEY0_PRES 	1             /* KEY0按下 */
#define WKUP_PRES   2             /* WK_UP按下 */

/******************************************************************************************/

void key_init(void);              /* 按键初始化函数 */
uint8_t key_scan(uint8_t mode);   /* 按键扫描函数 */

#endif







