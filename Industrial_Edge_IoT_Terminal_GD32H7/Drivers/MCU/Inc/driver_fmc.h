/*!
 * 文件名称： driver_fmc.h
 * 描    述： fmc 驱动文件
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
#ifndef DRIVER_FMC_H
#define DRIVER_FMC_H
#include "public.h"

#define FLAG_PAGE_SIZE 0x1000

void fmc_write_data(uint32_t write_start_addr, uint8_t *data_buf, uint16_t data_lengh);
uint8_t fmc_read_data(uint32_t write_read_addr);

#endif /* DRIVER_FMC_H*/
