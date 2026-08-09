/*!
 * 文件名称： driver_exmc.h
 * 描    述： exmc 驱动文件
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

#ifndef DRIVER_EXMC_H
#define DRIVER_EXMC_H
#include "public.h"

#define EXMC_BANK0_NORSRAM_REGIONx_ADDR(NEx)       ((uint32_t)0x60000000+64*1024*1024*NEx)

#define EXMC_BANK0_NORSRAM_REGION0_ADDR            EXMC_BANK0_NORSRAM_REGIONx_ADDR(0)
#define EXMC_BANK0_NORSRAM_REGION1_ADDR            EXMC_BANK0_NORSRAM_REGIONx_ADDR(1)
#define EXMC_BANK0_NORSRAM_REGION2_ADDR            EXMC_BANK0_NORSRAM_REGIONx_ADDR(2)
#define EXMC_BANK0_NORSRAM_REGION3_ADDR            EXMC_BANK0_NORSRAM_REGIONx_ADDR(3)

void driver_exmc_norsram_init(uint32_t norsram_region);
void driver_exmc_lcd_init(uint32_t norsram_region);
Drv_Err driver_exmc_sdram_init(uint32_t sdram_device);

    

#endif /* GPIO_H*/
