/*!
 * 文件名称：lcdcd_font.h
 * 描    述：  lcd字库
 * 版本：      2023-12-03, V1.0
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

#ifndef __lcd_FONT_H
#define __lcd_FONT_H

/* 字体大小枚举 */
typedef enum {FONT_ASCII_12_6 = 12,FONT_ASCII_16_8=16,FONT_ASCII_24_12=24,FONT_ASCII_32_16=32} FONT_ASCII;

/* 12*6 ASCII字符集点阵 */
extern const unsigned char ascii_12_6[95][12];

/* 16*8 ASCII字符集点阵 */
extern const unsigned char ascii_16_8[95][16];;

/* 24*12 ASICII字符集点阵 */
extern const unsigned char ascii_24_12[95][36];

/* 32*16 ASCII字符集点阵 */
extern const unsigned char ascii_32_16[95][64];



#endif





















