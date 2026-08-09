/*!
 * 文件名称： driver_wdog.c
 * 描    述： wdog 驱动文件
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

#include "public.h"
#include "driver_wdog.h"
#include "driver_gpio.h"

/*!
* 说明     看门狗配置函数
* 输入[1]  reload_value： 看门狗重载值
* 输入[2]  prescaler_div：看门狗预分频值
* 返回值   无
*/
void drive_fwdgt_config(uint16_t reload_value, uint8_t prescaler_div)
{
    rcu_osci_on(RCU_IRC32K);      											/* 开启内部40K时钟 */ 
    /* 等待时钟ready */
    while(SUCCESS != rcu_osci_stab_wait(RCU_IRC32K)){
    }
		fwdgt_config(reload_value,prescaler_div);						/*配置看门狗参数*/
    fwdgt_enable();																			/*开启看门狗*/
}

/*!
* 说明     看门狗喂狗函数
* 输入     无
* 返回值   无
*/
void driver_feed_fwdgt(void)
{
	fwdgt_counter_reload(); 															/* 重装载计数器 */
}
