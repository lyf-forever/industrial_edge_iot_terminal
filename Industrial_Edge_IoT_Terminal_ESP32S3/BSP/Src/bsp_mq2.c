#include "bsp_mq2.h"

#if Mq2Use
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc_cal.h"
#include <math.h>

static float mq2_cal_ro = 0.0f;
/* ADC 特性结构体，全局保存避免泄漏 */
static esp_adc_cal_characteristics_t mq2_adc_chars;

/******************************************************************
 * 函数名称：mq2_drv_init
 * 函数说明：mq2片上初始化 - ADC
 * 函数形参：void
 * 函数返回：void
 * 作者：Lyf
 * 备注：无
 ******************************************************************/
void mq2_drv_init(void *arg)
{
    /* 配置ADC分辨率 */
    adc1_config_width(MQ2_ADC_BITWIDTH);  // 12位分辨率
    /* 配置ADC通道的衰减系数 */
    /* ADC_ATTEN_DB_0:表示参考电压为1.1V
       ADC_ATTEN_DB_2_5:表示参考电压为1.5V
       ADC_ATTEN_DB_6:表示参考电压为2.2V
       ADC_ATTEN_DB_11:表示参考电压为3.3V */
    adc1_config_channel_atten( MQ2_ADC_CHANNEL,MQ2_ADC_ATTEN); // 设置通道5和3.3V参考电压
    /* ADC特性校准 */
    esp_adc_cal_characterize(MQ2_ADC_UNIT, MQ2_ADC_ATTEN, MQ2_ADC_BITWIDTH, MQ2_DFT_VREF, &mq2_adc_chars);
}

/******************************************************************
 * 函数名称：mq2_get_rawValue
 * 函数说明：对DMA保存的数据进行平均值计算后输出
 * 函数形参：uint16_t adc原始值
 * 函数返回：对应扫描的ADC采样平均原始值
 * 作者：Lyf
 * 备注：无
******************************************************************/
static uint16_t mq2_get_rawValue(void)
{
    uint16_t adcVal = 0;

    /* 因为采集 SAMPLES 次，故循环 SAMPLES 次 */
    for(uint8_t i = 0; i < MQ2_ADC_SAMPLES; i++)
    {
        /* 累加 */
        adcVal += adc1_get_raw(MQ2_ADC_CHANNEL);
        delay_us(100);
    }
    /* 求平均值 */
    adcVal /= MQ2_ADC_SAMPLES;

    return adcVal;
}

#if 0
/******************************************************************
 * 函数名称：mq2_calibrate_r0
 * 函数说明：在洁净空气内标定R0
 * 函数形参：void
 * 函数返回：void
 * 作者：Lyf
 * 备注：调用前确保传感器已在洁净空气中预热至少 1 分钟
******************************************************************/
static void mq2_calibrate_r0(void)
{
    float rs_sum = 0.0f;
    uint16_t adc_raw;
    float vrl, rs;

    for (uint8_t i = 0; i < MQ2_CALI_R0_TIMES; i++) {
        /* 单次读取 ADC 值（标定期间使用单次模式，不影响 DMA 循环）*/
        adc_raw = adc1_get_raw(MQ2_ADC_CHANNEL);
        vrl = (float)adc_raw / (float)(1 << MQ2_ADC_BITWIDTH - 1) * MQ2_VREF;
        
        if (vrl > 0.01f) rs = (MQ2_VC - vrl) * MQ2_RL / vrl;
        else rs = 999.9f;   /* 异常值 */
    
        rs_sum += rs;

        /* 延时 100us */
        delay_us(100);
    }
    mq2_cal_ro = rs_sum / MQ2_CALI_R0_TIMES;
}
#endif

/******************************************************************
 * 函数名称：mq2_getConcentration
 * 函数说明：获取mq2的最终输出浓度/ppm
 * 函数形参：void
 * 函数返回：float 浓度值
 * 作者：Lyf
 * 备注：无
******************************************************************/
float mq2_getConcentration(void)
{
    uint16_t rawVal = mq2_get_rawValue();
    float rs = 0.0f;
    /* 计算电压 (V) */
    float vrl = (float)rawVal / (float)(1 << (MQ2_ADC_BITWIDTH - 1)) * MQ2_VREF;
    /* 计算传感器电阻 Rs (kΩ) */
    if (vrl > 0.01f) 
        rs = (MQ2_VC - vrl) * MQ2_RL / vrl;
    else 
        rs = 999.9f;
    /* 计算 ppm 浓度 */
    float conc = pow(MQ2_SMOKE_A * 2 / rs, MQ2_SMOKE_B) * 100 + MQ2_PPM_OFFSET;
    return conc;
}

/******************************************************************
 * 函数名称：mq2_getPercentage
 * 函数说明：读取MQ2值，并且返回百分比
 * 函数形参：无
 * 函数返回：返回百分比
 * 作者：Lyf
 * 备注：无
******************************************************************/
uint16_t mq2_getPercentage(void)
{
    uint16_t adc_max = 1 << (MQ2_ADC_BITWIDTH - 1);
    uint16_t adc_new = 0;
    uint16_t Percentage_value = 0;

    adc_new = mq2_get_rawValue();

    Percentage_value = ((float)adc_new/adc_max) * 100;
    return Percentage_value;
}

#endif 

