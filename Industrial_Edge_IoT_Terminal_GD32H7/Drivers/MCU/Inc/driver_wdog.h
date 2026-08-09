/*!
 * 文件名称： driver_wdog.h
 * 描    述： wdog 驱动文件
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
#ifndef DRIVER_WDOG_H
#define DRIVER_WDOG_H
#include "public.h"
#include "driver_gpio.h"

void drive_fwdgt_config(uint16_t reload_value, uint8_t prescaler_div);
void driver_feed_fwdgt(void);

#endif /* DRIVER_WDOG_H*/
