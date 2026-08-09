#ifndef __SYS_H_
#define __SYS_H_

#include <stdio.h>
#include <stdint.h>
#include "esp_log.h"

/* 结构体 */


/* 传感器使用 */
#define SensorUse    1     /* 0: 不使用传感器 1：使用传感器 */
/* 传感器详情 */
#define Mq2Use       1     /* 0: 不使用MQ2 1：使用MQ2 */

/* API declare */
void delay_us(uint32_t us);

#endif 