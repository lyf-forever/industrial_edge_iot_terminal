#ifndef __BSP_MQ2_H_
#define __BSP_MQ2_H_

#include "sys.h"
#include "driver/gpio.h"
#include "driver/adc.h"

#if Mq2Use  /* 使用MQ2气体浓度传感器 */
/* ======================rel definition ============================*/
#define TAG "MQ2"
/* 硬件引脚与 ADC 配置 */
#define MQ2_ADC_UNIT       ADC_UNIT_1          // 使用 ADC1
#define MQ2_ADC_CHANNEL    ADC_CHANNEL_5       // GPIO6 对应 ADC1_CH5
#define MQ2_ADC_ATTEN      ADC_ATTEN_DB_11     // ADC衰减系数 这里选择 0 ~ 3.3V 量程
#define MQ2_ADC_BITWIDTH   ADC_WIDTH_BIT_12    // 12 位分辨率 (0~4095)
#define MQ2_ADC_SAMPLES    30                  // 采样次数
#define MQ2_CALI_R0_TIMES  10                  // 标定R0电阻采样次数
#define MQ2_VREF           3.3f                // ADC基准电压(V)
#define MQ2_VC             5.0f                // MQ2回路电压(V)
#define MQ2_RL             0.5f                // 负载电阻(kΩ)
#define MQ2_DFT_VREF       1100                // 默认参考电压/mV
#define MQ2_ADC_IO_NUM     6                   // MQ2 ADC通道IO号
#define MQ2_ADC_IO         GPIO_NUM_6          // MQ2 ADC通道IO

/* 烟雾特性曲线拟合系数 */
#define MQ2_SMOKE_A        11.5428f
#define MQ2_SMOKE_B        0.6549f

#define MQ2_PPM_OFFSET     0.0f                // 浓度偏移(ppm)

/* ======================rel definition ============================*/

/* =========================API declare=============================*/
void mq2_drv_init(void);
float mq2_getConcentration(void);
uint16_t mq2_getPercentage(void);

/* =========================API declare=============================*/

#endif /* #if Mq2_Use */

#endif