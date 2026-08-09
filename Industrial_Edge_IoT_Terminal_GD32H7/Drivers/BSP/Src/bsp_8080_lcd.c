/*!
 * 文件名称：  bsp_8080_lcd.c
 * 描    述：  bsp lcd interface
 * 版本：      2023-12-03, V1.0
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

#include "bsp_8080_lcd.h"
#include "public.h"
#include "driver_gpio.h"
#include "driver_i2c.h"
#include "driver_exmc.h"
#include "driver_timer.h"
#include "driver_dma.h"
#include "bsp_8080_lcd_init.h"
#include "stdarg.h"

//用户配置定义区
#define BACK_LIGHT_DUTY 80

TIMER_CH_DEF(LCD_8080_BL,TIMER3,1,TIMER_CH_PWM_HIGH,D,13,AF_PP,GPIO_AF_2);

#define EXMC_Ax  12
AFIO_DEF(EXMC_Ax_GPIO,G,2,AF_PP,GPIO_AF_12);

#define EXMC_NEx 1
AFIO_DEF(EXMC_NEx_GPIO,G,9,AF_PP,GPIO_AF_12);

GPIO_DEF(LCD_8080_RST,D,12,OUT_PP,RESET,NULL);



//驱动内部定义区
#define EXMC_LCD_D  REG16(((uint32_t)(EXMC_BANK0_NORSRAM_REGIONx_ADDR(EXMC_NEx)))|BIT(EXMC_Ax)*2)
#define EXMC_LCD_C  REG16(((uint32_t)(EXMC_BANK0_NORSRAM_REGIONx_ADDR(EXMC_NEx))))


typde_8080_lcd_printf_parameter_struct bsp_8080_lcd_pritnf_parameter;


/* 管理LCD重要参数 */
typde_8080_lcd_parameter_struct bsp_8080_lcd_parameter;


static __IO uint8_t lcd_backlight_duty=0;

static uint16_t bsp_8080_lcd_read_data(void);



static void bsp_8080_lcd_port_init(void)
{    
    driver_timer_channel_init(&LCD_8080_BL,10000000,100);//1M/1000 pwm ck=1K
       
    driver_timer_pwm_on(&LCD_8080_BL);
    
    driver_dma_mem_init(&DMA_MEM);
    
    driver_gpio_general_init(&EXMC_Ax_GPIO);
    driver_gpio_general_init(&EXMC_NEx_GPIO);    
    
    driver_exmc_lcd_init(EXMC_LCD(EXMC_NEx));
}

void bsp_8080_lcd_backlight_on(void)
{    
    driver_timer_pwm_on(&LCD_8080_BL);
    driver_timer_pwm_duty_set(&LCD_8080_BL,lcd_backlight_duty);    
}

void bsp_8080_lcd_backlight_off(void)
{   
    driver_timer_pwm_off(&LCD_8080_BL);    
    driver_timer_pwm_duty_set(&LCD_8080_BL,0);
}

void bsp_8080_lcd_backlight_duty_set(uint8_t backlight_value)
{    
    lcd_backlight_duty=backlight_value;
    driver_timer_pwm_duty_set(&LCD_8080_BL,lcd_backlight_duty);
}


/**
 * 说明       初始化LCD
 *   @note      该初始化函数可以初始化各种型号的LCD(详见本.c文件最前面的描述)
 *
 * 输入       无
 * 返回值      无
 */
uint32_t bsp_8080_lcd_init(void)
{
    
    bsp_8080_lcd_port_init();

    driver_gpio_general_init(&LCD_8080_RST);
    
    driver_gpio_pin_reset(&LCD_8080_RST);
    delay_ms(10);
    driver_gpio_pin_set(&LCD_8080_RST);
    
    delay_ms(10);    
    bsp_8080_lcd_backlight_duty_set(BACK_LIGHT_DUTY);
    
    bsp_8080_lcd_backlight_on();          /* 点亮背光 */    
    
    /* LCD的画笔颜色和背景色 */
    bsp_8080_lcd_parameter.g_point_color = WHITE;    /* 画笔颜色 */
    bsp_8080_lcd_parameter.g_back_color  = BLACK;    /* 背景色 */    
    
    delay_ms(1);     /* 初始化FSMC后,必须等待一定时间才能开始初始化 */

    /* 尝试9341 ID的读取 */
    bsp_8080_lcd_wr_regno(0XD3);
    bsp_8080_lcd_parameter.id = bsp_8080_lcd_read_data();  /* dummy read */
    bsp_8080_lcd_parameter.id = bsp_8080_lcd_read_data();  /* 读到0X00 */
    bsp_8080_lcd_parameter.id = bsp_8080_lcd_read_data();  /* 读取0X93 */
    bsp_8080_lcd_parameter.id <<= 8;
    bsp_8080_lcd_parameter.id |= bsp_8080_lcd_read_data(); /* 读取0X41 */


    if (bsp_8080_lcd_parameter.id == 0X9488)
    {
        lcd_ex_ili9488_reginit();   /* 执行ILI9388初始化 */
    }     
    else if (bsp_8080_lcd_parameter.id == 0X9341)
    {
        lcd_ex_ili9341_reginit();   /* 执行ILI9341初始化 */
    }else{
        bsp_8080_lcd_parameter.id = 0X9488;
        bsp_8080_lcd_display_dir(0); /* 默认为竖屏 */
        bsp_8080_lcd_scan_dir(DFT_SCAN_DIR);     /* 默认扫描方向 */        
        return DRV_ERROR;
    }

    bsp_8080_lcd_display_dir(0); /* 默认为竖屏 */
    bsp_8080_lcd_scan_dir(DFT_SCAN_DIR);     /* 默认扫描方向 */
    bsp_8080_lcd_clear(WHITE);
    
    return DRV_SUCCESS;
}



/**
 * 说明       LCD写数据
 * 输入       data: 要写入的数据
 * 返回值      无
 */
void bsp_8080_lcd_wr_data(__IO uint16_t data)
{
//    delay_sysclk(1);            
    EXMC_LCD_D = data;
}

/**
 * 说明       LCD写寄存器编号/地址函数
 * 输入       regno: 寄存器编号/地址
 * 返回值      无
 */
void bsp_8080_lcd_wr_regno(__IO uint16_t regno)
{
//    delay_sysclk(1);        
    EXMC_LCD_C = regno;   /* 写入要写的寄存器序号 */
}

/**
 * 说明       LCD写寄存器
 * 输入       regno:寄存器编号/地址
 * 输入       data:要写入的数据
 * 返回值      无
 */
void bsp_8080_lcd_write_reg(__IO uint16_t regno,__IO uint16_t data)
{
//    delay_sysclk(1);     
    EXMC_LCD_C = regno;   /* 写入要写的寄存器序号 */
//    delay_sysclk(1);     
    EXMC_LCD_D = data;    /* 写入数据 */
}

/**
 * 说明       LCD读数据
 * 输入       无
 * 返回值      读取到的数据
 */
static uint16_t bsp_8080_lcd_read_data(void)
{
//    delay_sysclk(1);
    return EXMC_LCD_D;
}

/**
 * 说明       读取个某点的颜色值
 * 输入       x,y:坐标
 * 返回值      此点的颜色(32位颜色,方便兼容LTDC)
 */
uint32_t bsp_8080_lcd_read_point(uint16_t x, uint16_t y)
{
    uint16_t r = 0, g = 0, b = 0;

    if (x >= bsp_8080_lcd_parameter.width || y >= bsp_8080_lcd_parameter.height)return 0;   /* 超过了范围,直接返回 */

    bsp_8080_lcd_set_cursor(x, y);       /* 设置坐标 */

    bsp_8080_lcd_wr_regno(0X2E);     /* 9341/5310/1963/7789 等发送读GRAM指令 */

    r = bsp_8080_lcd_read_data();          /* 假读(dummy read) */

    if (bsp_8080_lcd_parameter.id == 0X1963)return r;   /* 1963 直接读就可以 */

    r = bsp_8080_lcd_read_data();          /* 实际坐标颜色 */

    /* 9341/5310/5510/7789 要分2次读出 */
    b = bsp_8080_lcd_read_data();
    g = r & 0XFF;               /* 对于 9341/5310/5510/7789, 第一次读取的是RG的值,R在前,G在后,各占8位 */
    g <<= 8;
    return (((r >> 11) << 11) | ((g >> 10) << 5) | (b >> 11));  /* 9341/5310/5510/7789 需要公式转换一下 */
}

/**
 * 说明       LCD开启显示
 * 输入       无
 * 返回值      无
 */
void bsp_8080_lcd_display_on(void)
{
    bsp_8080_lcd_wr_regno(0X29);     /* 开启显示 */
}

/**
 * 说明       LCD关闭显示
 * 输入       无
 * 返回值      无
 */
void bsp_8080_lcd_display_off(void)
{
    bsp_8080_lcd_wr_regno(0X28);     /* 关闭显示 */
}

/**
 * 说明       设置光标位置(对RGB屏无效)
 * 输入       x,y: 坐标
 * 返回值      无
 */
void bsp_8080_lcd_set_cursor(uint16_t x, uint16_t y)
{
    bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setxcmd);
    bsp_8080_lcd_wr_data(x >> 8);
    bsp_8080_lcd_wr_data(x & 0XFF);
    bsp_8080_lcd_wr_data(bsp_8080_lcd_parameter.width >> 8);
    bsp_8080_lcd_wr_data(bsp_8080_lcd_parameter.width & 0XFF);        
    bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setycmd);
    bsp_8080_lcd_wr_data(y >> 8);
    bsp_8080_lcd_wr_data(y & 0XFF);
    bsp_8080_lcd_wr_data(bsp_8080_lcd_parameter.height >> 8);
    bsp_8080_lcd_wr_data(bsp_8080_lcd_parameter.height & 0XFF);          

}

/**
 * 说明       设置LCD的自动扫描方向(对RGB屏无效)
 *   @note
 *              9341/5310/5510/1963/7789等IC已经实际测试
 *              注意:其他函数可能会受到此函数设置的影响(尤其是9341),
 *              所以,一般设置为L2R_U2D即可,如果设置为其他扫描方式,可能导致显示不正常.
 *
 * 输入       dir:0~7,代表8个方向(具体定义见lcd.h)
 * 返回值      无
 */
void bsp_8080_lcd_scan_dir(uint8_t dir)
{
    uint16_t regval = 0;
    uint16_t dirreg = 0;
 
    /* 根据扫描方式 设置 0X36/0X3600 寄存器 bit 5,6,7 位的值 */
    switch (dir)
    {
        case L2R_U2D:/* 从左到右,从上到下 */
            regval |= (0 << 7) | (0 << 6) | (0 << 5);
            break;

        case L2R_D2U:/* 从左到右,从下到上 */
            regval |= (1 << 7) | (0 << 6) | (0 << 5);
            break;

        case R2L_U2D:/* 从右到左,从上到下 */
            regval |= (0 << 7) | (1 << 6) | (0 << 5);
            break;

        case R2L_D2U:/* 从右到左,从下到上 */
            regval |= (1 << 7) | (1 << 6) | (0 << 5);
            break;

        case U2D_L2R:/* 从上到下,从左到右 */
            regval |= (0 << 7) | (0 << 6) | (1 << 5);
            break;

        case U2D_R2L:/* 从上到下,从右到左 */
            regval |= (0 << 7) | (1 << 6) | (1 << 5);
            break;

        case D2U_L2R:/* 从下到上,从左到右 */
            regval |= (1 << 7) | (0 << 6) | (1 << 5);
            break;

        case D2U_R2L:/* 从下到上,从右到左 */
            regval |= (1 << 7) | (1 << 6) | (1 << 5);
            break;
    }

    dirreg = 0X36;  /* 对绝大部分驱动IC, 由0X36寄存器控制 */


    /* 9341 & 7789 要设置BGR位 */
    if (bsp_8080_lcd_parameter.id == 0X9341 || bsp_8080_lcd_parameter.id == 0X9488 || bsp_8080_lcd_parameter.id == 0X7789)
    {
        regval |= 0X08;
    }

    bsp_8080_lcd_write_reg(dirreg, regval);

    /* 设置显示区域(开窗)大小 */
    if (bsp_8080_lcd_parameter.id == 0X5510)
    {
        bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setxcmd);
        bsp_8080_lcd_wr_data(0);
        bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setxcmd + 1);
        bsp_8080_lcd_wr_data(0);
        bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setxcmd + 2);
        bsp_8080_lcd_wr_data((bsp_8080_lcd_parameter.width - 1) >> 8);
        bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setxcmd + 3);
        bsp_8080_lcd_wr_data((bsp_8080_lcd_parameter.width - 1) & 0XFF);
        bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setycmd);
        bsp_8080_lcd_wr_data(0);
        bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setycmd + 1);
        bsp_8080_lcd_wr_data(0);
        bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setycmd + 2);
        bsp_8080_lcd_wr_data((bsp_8080_lcd_parameter.height - 1) >> 8);
        bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setycmd + 3);
        bsp_8080_lcd_wr_data((bsp_8080_lcd_parameter.height - 1) & 0XFF);
    }
    else
    {
        bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setxcmd);
        bsp_8080_lcd_wr_data(0);
        bsp_8080_lcd_wr_data(0);
        bsp_8080_lcd_wr_data((bsp_8080_lcd_parameter.width - 1) >> 8);
        bsp_8080_lcd_wr_data((bsp_8080_lcd_parameter.width - 1) & 0XFF);
        bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setycmd);
        bsp_8080_lcd_wr_data(0);
        bsp_8080_lcd_wr_data(0);
        bsp_8080_lcd_wr_data((bsp_8080_lcd_parameter.height - 1) >> 8);
        bsp_8080_lcd_wr_data((bsp_8080_lcd_parameter.height - 1) & 0XFF);
    }
}

/**
 * 说明       切换显示控制模式，直接修改LCD模式还是 RAM刷屏模式
 * 输入       display_mode:LCD_MODE\SRAM_MODE
 * 输入       display_ram:SRAM_MODE模式时的显存地址 
 * 返回值      无
 */
void bsp_8080_lcd_mode_config(DISPLAY_MODE display_mode,uint16_t* display_ram)
{
    if(display_mode!=LCD_MODE){
        bsp_8080_lcd_parameter.display_ram=display_ram;    
    }
    bsp_8080_lcd_parameter.display_mode=display_mode;
}

/**
 * 说明       画点
 * 输入       x,y: 坐标
 * 输入       color: 点的颜色(32位颜色,方便兼容LTDC)
 * 返回值      无
 */
void bsp_8080_lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
    if(x>bsp_8080_lcd_parameter.width || y>bsp_8080_lcd_parameter.height)
    {
        return;
    }
    
    if(bsp_8080_lcd_parameter.display_mode == LCD_MODE)
    {
        bsp_8080_lcd_set_cursor(x, y);       /* 设置光标位置 */
        EXMC_LCD_C = bsp_8080_lcd_parameter.wramcmd;    /* 开始写入GRAM */
        EXMC_LCD_D = color;
    }else
    {
        *(__IO uint16_t*)(((uint32_t)(bsp_8080_lcd_parameter.display_ram)) + 2*((bsp_8080_lcd_parameter.width*y) + x)) = color;        
    }
}

/**
 * 说明       SSD1963背光亮度设置函数
 * 输入       pwm: 背光等级,0~100.越大越亮.
 * 返回值      无
 */
void bsp_8080_lcd_ssd_backlight_set(uint8_t pwm)
{
    bsp_8080_lcd_wr_regno(0xBE);         /* 配置PWM输出 */
    bsp_8080_lcd_wr_data(0x05);          /* 1设置PWM频率 */
    bsp_8080_lcd_wr_data(pwm * 2.55);    /* 2设置PWM占空比 */
    bsp_8080_lcd_wr_data(0x01);          /* 3设置C */
    bsp_8080_lcd_wr_data(0xFF);          /* 4设置D */
    bsp_8080_lcd_wr_data(0x00);          /* 5设置E */
    bsp_8080_lcd_wr_data(0x00);          /* 6设置F */
}

/**
 * 说明       设置LCD显示方向
 * 输入       dir:0,竖屏; 1,横屏
 * 返回值      无
 */
void bsp_8080_lcd_display_dir(uint8_t dir)
{
    bsp_8080_lcd_parameter.dir = dir;   /* 竖屏/横屏 */

    if (dir == 0)       /* 竖屏 */
    {
        bsp_8080_lcd_parameter.width = 240;
        bsp_8080_lcd_parameter.height = 320;

        if (bsp_8080_lcd_parameter.id == 0X1963)
        {
            bsp_8080_lcd_parameter.wramcmd = 0X2C;  /* 设置写入GRAM的指令 */
            bsp_8080_lcd_parameter.setxcmd = 0X2B;  /* 设置写X坐标指令 */
            bsp_8080_lcd_parameter.setycmd = 0X2A;  /* 设置写Y坐标指令 */
            bsp_8080_lcd_parameter.width = 480;     /* 设置宽度480 */
            bsp_8080_lcd_parameter.height = 800;    /* 设置高度800 */
        }
        else   /* 其他IC, 包括: 9341 / 5310 / 7789等IC */
        {
            bsp_8080_lcd_parameter.wramcmd = 0X2C;
            bsp_8080_lcd_parameter.setxcmd = 0X2A;
            bsp_8080_lcd_parameter.setycmd = 0X2B;
        }

        if ( bsp_8080_lcd_parameter.id == 0X9488)    /* 如果是5310 则表示是 320*480分辨率 */
        {
            bsp_8080_lcd_parameter.width = 320;
            bsp_8080_lcd_parameter.height = 480;
        }    
    }
    else                /* 横屏 */
    {
        bsp_8080_lcd_parameter.width = 320;         /* 默认宽度 */
        bsp_8080_lcd_parameter.height = 240;        /* 默认高度 */

        if (bsp_8080_lcd_parameter.id == 0X1963)
        {
            bsp_8080_lcd_parameter.wramcmd = 0X2C;  /* 设置写入GRAM的指令 */
            bsp_8080_lcd_parameter.setxcmd = 0X2A;  /* 设置写X坐标指令 */
            bsp_8080_lcd_parameter.setycmd = 0X2B;  /* 设置写Y坐标指令 */
            bsp_8080_lcd_parameter.width = 800;     /* 设置宽度800 */
            bsp_8080_lcd_parameter.height = 480;    /* 设置高度480 */
        }
        else   /* 其他IC, 包括: 9341 / 5310 / 7789等IC */
        {
            bsp_8080_lcd_parameter.wramcmd = 0X2C;
            bsp_8080_lcd_parameter.setxcmd = 0X2A;
            bsp_8080_lcd_parameter.setycmd = 0X2B;
        }


        if (bsp_8080_lcd_parameter.id == 0X9488)    /* 如果是5310 则表示是 320*480分辨率 */
        {
            bsp_8080_lcd_parameter.width = 480;
            bsp_8080_lcd_parameter.height = 320;
        }
    }
}

/**
 * 说明       设置窗口(对RGB屏无效),并自动设置画点坐标到窗口左上角(sx,sy).
 * 输入       sx,sy:窗口起始坐标(左上角)
 * 输入       width,height:窗口宽度和高度,必须大于0!!
 *   @note      窗体大小:width*height.
 *
 * 返回值      无
 */
void bsp_8080_lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
    uint16_t twidth, theight;
    
    if(sx>bsp_8080_lcd_parameter.width || sy>bsp_8080_lcd_parameter.height )
    {
        return;
    }
    
    twidth = sx + width - 1;
    theight = sy + height - 1;

    if (bsp_8080_lcd_parameter.id == 0X1963 && bsp_8080_lcd_parameter.dir != 1)    /* 1963竖屏特殊处理 */
    {
        sx = bsp_8080_lcd_parameter.width - width - sx;
        height = sy + height - 1;
        bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setxcmd);
        bsp_8080_lcd_wr_data(sx >> 8);
        bsp_8080_lcd_wr_data(sx & 0XFF);
        bsp_8080_lcd_wr_data((sx + width - 1) >> 8);
        bsp_8080_lcd_wr_data((sx + width - 1) & 0XFF);
        bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setycmd);
        bsp_8080_lcd_wr_data(sy >> 8);
        bsp_8080_lcd_wr_data(sy & 0XFF);
        bsp_8080_lcd_wr_data(height >> 8);
        bsp_8080_lcd_wr_data(height & 0XFF);
    }
    else    /* 9341/5310/7789/1963横屏 等 设置窗口 */
    {
        bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setxcmd);
        bsp_8080_lcd_wr_data(sx >> 8);
        bsp_8080_lcd_wr_data(sx & 0XFF);
        bsp_8080_lcd_wr_data(twidth >> 8);
        bsp_8080_lcd_wr_data(twidth & 0XFF);
        bsp_8080_lcd_wr_regno(bsp_8080_lcd_parameter.setycmd);
        bsp_8080_lcd_wr_data(sy >> 8);
        bsp_8080_lcd_wr_data(sy & 0XFF);
        bsp_8080_lcd_wr_data(theight >> 8);
        bsp_8080_lcd_wr_data(theight & 0XFF);
    }
}


/**
 * 说明       清屏函数
 * 输入       color: 要清屏的颜色
 * 返回值      无
 */
void bsp_8080_lcd_clear(uint16_t color)
{
    uint32_t index = 0;
    uint32_t totalpoint = bsp_8080_lcd_parameter.width;
    bsp_8080_lcd_parameter.g_back_color = color;
    
    bsp_8080_lcd_parameter.g_point_color = ~color;

    totalpoint *= bsp_8080_lcd_parameter.height;    /* 得到总点数 */
    bsp_8080_lcd_set_cursor(0x00, 0x0000);   /* 设置光标位置 */
    EXMC_LCD_C = bsp_8080_lcd_parameter.wramcmd;        /* 开始写入GRAM */

    for (index = 0; index < totalpoint; index++)
    {
        EXMC_LCD_D = color;
   }    
}

/**
 * 说明       在指定区域内填充单个颜色
 * 输入       (sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex - sx + 1) * (ey - sy + 1)
 * 输入       color: 要填充的颜色(32位颜色,方便兼容LTDC)
 * 返回值      无
 */
void bsp_8080_lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
    uint16_t height, width,x,y;
    uint32_t i;
    if(sx>bsp_8080_lcd_parameter.width || ex>bsp_8080_lcd_parameter.width || sy>bsp_8080_lcd_parameter.height || ey>bsp_8080_lcd_parameter.height)
    {
        return;
    }
    
    width = ex - sx + 1;            /* 得到填充的宽度 */
    height = ey - sy + 1;           /* 高度 */

    if(bsp_8080_lcd_parameter.display_mode == LCD_MODE)
    {    
        bsp_8080_lcd_set_window(sx,sy,width,height);
        EXMC_LCD_C = bsp_8080_lcd_parameter.wramcmd;    
        for(i = 0; i < height*width; i++)
        {
            EXMC_LCD_D = color;     
        }
    }
    else
    {    
        for(y = sy; y < sy + height; y++){      
            for(x = sx; x < sx + width; x++){ 
                *(__IO uint16_t*)( (uint32_t)(bsp_8080_lcd_parameter.display_ram) + 2*((bsp_8080_lcd_parameter.width*y) + x) ) = color;
            }                
        } 
    }        
      
//    if(bsp_8080_lcd_parameter.display_mode == LCD_MODE)
//    {
//        bsp_8080_lcd_set_cursor(x, y);       /* 设置光标位置 */
//        EXMC_LCD_C = bsp_8080_lcd_parameter.wramcmd;    /* 开始写入GRAM */
//        EXMC_LCD_D = color;
//    }else
//    {
//        *(__IO uint16_t*)(((uint32_t)(&bsp_8080_lcd_parameter.display_ram)) + 2*((bsp_8080_lcd_parameter.width*y) + x)) = color;        
//    }    
}

/**
 * 说明       在指定区域内填充指定颜色块
 * 输入       (sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex - sx + 1) * (ey - sy + 1)
 * 输入       color: 要填充的颜色数组首地址
 * 返回值      无
 */
void bsp_8080_lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    uint16_t height, width;
    if(sx>bsp_8080_lcd_parameter.width || ex>bsp_8080_lcd_parameter.width || sy>bsp_8080_lcd_parameter.height || ey>bsp_8080_lcd_parameter.height)
    {
        return;
    }    
//    uint32_t i;
    width = ex - sx + 1;            /* 得到填充的宽度 */
    height = ey - sy + 1;           /* 高度 */
    
    
    if(bsp_8080_lcd_parameter.display_mode == LCD_MODE)
    {   
        bsp_8080_lcd_set_window(sx,sy,width,height);
        EXMC_LCD_C = bsp_8080_lcd_parameter.wramcmd;
        driver_dma_mem_to_exmclcd_start((void*)&EXMC_LCD_D,&color[0],DMA_Width_16BIT,height*width);     
//        for(uin32_t i = 0; i < height*width; i++)
//        {
//            EXMC_LCD_D = color[i];     
//        }
    }    
    else 
    {       
        for(uint16_t y = sy; y <= ey; y++)
        {
            for(uint16_t x = sx; x <= ex; x++)
            {
                bsp_8080_lcd_parameter.display_ram[y*width+x]=color[(y-sy)*width+(x-sx)];
            }
        }
    }
    
}

/**
 * 说明       刷新LCD
 * 输入       (sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex - sx + 1) * (ey - sy + 1)
 * 输入       color: 要填充的颜色数组首地址
 * 返回值      无
 */
void bsp_8080_lcd_sram_mode_updata(void)
{
    if(bsp_8080_lcd_parameter.display_mode == SRAM_MODE)
    {       
        bsp_8080_lcd_color_fill(0,0,bsp_8080_lcd_parameter.width-1,bsp_8080_lcd_parameter.height-1,bsp_8080_lcd_parameter.display_ram);
        
        bsp_8080_lcd_set_window(0,0,bsp_8080_lcd_parameter.width,bsp_8080_lcd_parameter.height);
        EXMC_LCD_C = bsp_8080_lcd_parameter.wramcmd;
        driver_dma_mem_to_exmclcd_start((void*)&EXMC_LCD_D,bsp_8080_lcd_parameter.display_ram,DMA_Width_16BIT,bsp_8080_lcd_parameter.height*bsp_8080_lcd_parameter.width);          
    }
}

/**
 * 说明       画线
 * 输入       x1,y1: 起点坐标
 * 输入       x2,y2: 终点坐标
 * 输入       color: 线的颜色
 * 返回值      无
 */
void bsp_8080_lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, row, col;
    delta_x = x2 - x1;          /* 计算坐标增量 */
    delta_y = y2 - y1;
    row = x1;
    col = y1;

    if (delta_x > 0)incx = 1;   /* 设置单步方向 */
    else if (delta_x == 0)incx = 0; /* 垂直线 */
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }

    if (delta_y > 0)incy = 1;
    else if (delta_y == 0)incy = 0; /* 水平线 */
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }

    if ( delta_x > delta_y)distance = delta_x;  /* 选取基本增量坐标轴 */
    else distance = delta_y;

    for (t = 0; t <= distance + 1; t++ )   /* 画线输出 */
    {
        bsp_8080_lcd_draw_point(row, col, color); /* 画点 */
        xerr += delta_x ;
        yerr += delta_y ;

        if (xerr > distance)
        {
            xerr -= distance;
            row += incx;
        }

        if (yerr > distance)
        {
            yerr -= distance;
            col += incy;
        }
    }
}

/**
 * 说明       画水平线
 * 输入       x0,y0: 起点坐标
 * 输入       len  : 线长度
 * 输入       color: 矩形的颜色
 * 返回值      无
 */
void bsp_8080_lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color)
{
    if ((len == 0) || (x > bsp_8080_lcd_parameter.width) || (y > bsp_8080_lcd_parameter.height))return;

    bsp_8080_lcd_fill(x, y, x + len - 1, y, color);
}

/**
 * 说明       画矩形
 * 输入       x1,y1: 起点坐标
 * 输入       x2,y2: 终点坐标
 * 输入       color: 矩形的颜色
 * 返回值      无
 */
void bsp_8080_lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    bsp_8080_lcd_draw_line(x1, y1, x2, y1, color);
    bsp_8080_lcd_draw_line(x1, y1, x1, y2, color);
    bsp_8080_lcd_draw_line(x1, y2, x2, y2, color);
    bsp_8080_lcd_draw_line(x2, y1, x2, y2, color);
}

/**
 * 说明       画圆
 * 输入       x,y  : 圆中心坐标
 * 输入       r    : 半径
 * 输入       color: 圆的颜色
 * 返回值      无
 */
void bsp_8080_lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int a, b;
    int di;
    a = 0;
    b = r;
    di = 3 - (r << 1);       /* 判断下个点位置的标志 */

    while (a <= b)
    {
        bsp_8080_lcd_draw_point(x0 + a, y0 - b, color);  /* 5 */
        bsp_8080_lcd_draw_point(x0 + b, y0 - a, color);  /* 0 */
        bsp_8080_lcd_draw_point(x0 + b, y0 + a, color);  /* 4 */
        bsp_8080_lcd_draw_point(x0 + a, y0 + b, color);  /* 6 */
        bsp_8080_lcd_draw_point(x0 - a, y0 + b, color);  /* 1 */
        bsp_8080_lcd_draw_point(x0 - b, y0 + a, color);
        bsp_8080_lcd_draw_point(x0 - a, y0 - b, color);  /* 2 */
        bsp_8080_lcd_draw_point(x0 - b, y0 - a, color);  /* 7 */
        a++;

        /* 使用Bresenham算法画圆 */
        if (di < 0)
        {
            di += 4 * a + 6;
        }
        else
        {
            di += 10 + 4 * (a - b);
            b--;
        }
    }
}

/**
 * 说明       填充实心圆
 * 输入       x0,y0: 圆中心坐标
 * 输入       r    : 半径
 * 输入       color: 圆的颜色
 * 返回值      无
 */
void bsp_8080_lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color)
{
    uint32_t i;
    uint32_t imax = ((uint32_t)r * 707) / 1000 + 1;
    uint32_t sqmax = (uint32_t)r * (uint32_t)r + (uint32_t)r / 2;
    uint32_t xr = r;

    bsp_8080_lcd_draw_hline(x - r, y, 2 * r, color);

    for (i = 1; i <= imax; i++)
    {
        if ((i * i + xr * xr) > sqmax)
        {
            /* draw lines from outside */
            if (xr > imax)
            {
                bsp_8080_lcd_draw_hline (x - i + 1, y + xr, 2 * (i - 1), color);
                bsp_8080_lcd_draw_hline (x - i + 1, y - xr, 2 * (i - 1), color);
            }

            xr--;
        }

        /* draw lines from inside (center) */
        bsp_8080_lcd_draw_hline(x - xr, y + i, 2 * xr, color);
        bsp_8080_lcd_draw_hline(x - xr, y - i, 2 * xr, color);
    }
}

/**
 * 说明       在指定位置显示一个字符
 * 输入       x,y  : 坐标
 * 输入       chr  : 要显示的字符:" "--->"~"
 * 输入       size : 字体大小 FONT_ASCII_12_6/FONT_ASCII_16_8/FONT_ASCII_24_12/FONT_ASCII_32_16
 * 输入       mode : 叠加方式(1); 非叠加方式(0);
 * 返回值      无
 */
void bsp_8080_lcd_show_char(uint16_t x, uint16_t y, char chr, FONT_ASCII size, uint8_t mode, uint16_t color,uint16_t back_color)
{
    uint8_t temp, t1, t;
    uint16_t y0 = y;
    uint8_t csize = 0;
    uint8_t *pfont = 0;

    csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); /* 得到字体一个字符对应点阵集所占的字节数 */
    chr = chr - ' ';    /* 得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库） */

    switch (size)
    {
        case FONT_ASCII_12_6:
            pfont = (uint8_t *)ascii_12_6[(unsigned char)chr];  /* 调用1206字体 */
            break;

        case FONT_ASCII_16_8:
            pfont = (uint8_t *)ascii_16_8[(unsigned char)chr];  /* 调用1608字体 */
            break;

        case FONT_ASCII_24_12:
            pfont = (uint8_t *)ascii_24_12[(unsigned char)chr];  /* 调用2412字体 */
            break;

        case FONT_ASCII_32_16:
            pfont = (uint8_t *)ascii_32_16[(unsigned char)chr];  /* 调用3216字体 */
            break;

        default:
            return ;
    }

    for (t = 0; t < csize; t++)
    {
        temp = pfont[t];    /* 获取字符的点阵数据 */

        for (t1 = 0; t1 < 8; t1++)   /* 一个字节8个点 */
        {
            if (temp & 0x80)        /* 有效点,需要显示 */
            {
                bsp_8080_lcd_draw_point(x, y, color);        /* 画点出来,要显示这个点 */
            }
            else if (mode == 0)     /* 无效点,不显示 */
            {
                bsp_8080_lcd_draw_point(x, y, back_color); /* 画背景色,相当于这个点不显示(注意背景色由全局变量控制) */
            }

            temp <<= 1; /* 移位, 以便获取下一个位的状态 */
            y++;

            if (y >= bsp_8080_lcd_parameter.height)return;  /* 超区域了 */

            if ((y - y0) == size)   /* 显示完一列了? */
            {
                y = y0; /* y坐标复位 */
                x++;    /* x坐标递增 */

                if (x >= bsp_8080_lcd_parameter.width)return;   /* x坐标超区域了 */

                break;
            }
        }
    }
}

/**
 * 说明       平方函数, m^n
 * 输入       m: 底数
 * 输入       n: 指数
 * 返回值      m的n次方
 */
static uint32_t bsp_8080_lcd_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;

    while (n--)result *= m;

    return result;
}

/**
 * 说明       显示len个数字
 * 输入       x,y : 起始坐标
 * 输入       num : 数值(0 ~ 2^32)
 * 输入       len : 显示数字的位数
 * 输入       size: 选择字体 FONT_ASCII_12_6/FONT_ASCII_16_8/FONT_ASCII_24_12/FONT_ASCII_32_16
 * 返回值      无
 */
void bsp_8080_lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, FONT_ASCII size, uint16_t color)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)   /* 按总显示位数循环 */
    {
        temp = (num / bsp_8080_lcd_pow(10, len - t - 1)) % 10;   /* 获取对应位的数字 */

        if (enshow == 0 && t < (len - 1))   /* 没有使能显示,且还有位要显示 */
        {
            if (temp == 0)
            {
                bsp_8080_lcd_show_char(x + (size / 2)*t, y, ' ', size, 0, color,bsp_8080_lcd_parameter.g_back_color);/* 显示空格,占位 */
                continue;   /* 继续下个一位 */
            }
            else
            {
                enshow = 1; /* 使能显示 */
            }

        }

        bsp_8080_lcd_show_char(x + (size / 2)*t, y, temp + '0', size, 0, color,bsp_8080_lcd_parameter.g_back_color); /* 显示字符 */
    }
}

/**
 * 说明       扩展显示len个数字(高位是0也显示)
 * 输入       x,y : 起始坐标
 * 输入       num : 数值(0 ~ 2^32)
 * 输入       len : 显示数字的位数
 * 输入       size: 选择字体 FONT_ASCII_12_6/FONT_ASCII_16_8/FONT_ASCII_24_12/FONT_ASCII_32_16
 * 输入       mode: 显示模式
 *              [7]:0,不填充;1,填充0.
 *              [6:1]:保留
 *              [0]:0,非叠加显示;1,叠加显示.
 *
 * 返回值      无
 */
void bsp_8080_lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, FONT_ASCII size, uint8_t mode, uint16_t color)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)   /* 按总显示位数循环 */
    {
        temp = (num / bsp_8080_lcd_pow(10, len - t - 1)) % 10;    /* 获取对应位的数字 */

        if (enshow == 0 && t < (len - 1))   /* 没有使能显示,且还有位要显示 */
        {
            if (temp == 0)
            {
                if (mode & 0X80)   /* 高位需要填充0 */
                {
                    bsp_8080_lcd_show_char(x + (size / 2)*t, y, '0', size, mode & 0X01, color,bsp_8080_lcd_parameter.g_back_color);  /* 用0占位 */
                }
                else
                {
                    bsp_8080_lcd_show_char(x + (size / 2)*t, y, ' ', size, mode & 0X01, color,bsp_8080_lcd_parameter.g_back_color);  /* 用空格占位 */
                }

                continue;
            }
            else
            {
                enshow = 1; /* 使能显示 */
            }

        }

        bsp_8080_lcd_show_char(x + (size / 2)*t, y, temp + '0', size, mode & 0X01, color,bsp_8080_lcd_parameter.g_back_color);
    }
}

/**
 * 说明       显示字符串
 * 输入       x,y         : 起始坐标
 * 输入       width,height: 区域大小
 * 输入       size        : 选择字体 FONT_ASCII_12_6/FONT_ASCII_16_8/FONT_ASCII_24_12/FONT_ASCII_32_16
 * 输入       p           : 字符串首地址
 * 返回值      无
 */
void bsp_8080_lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, FONT_ASCII size, char *p, uint16_t color)
{
    uint8_t x0 = x;
    width += x;
    height += y;

    while( ((*p <= '~') && (*p >= ' ')) || (*p =='\r') || (*p =='\n') )   /* 判断是不是非法字符! */
    {
        if((*p =='\r'))
        {
            x = x0;
        }        
        else if((*p =='\n'))
        {
            y += size;        
        }
        else if (x >= width)
        {
            x = x0;
            y += size;
        }
        if (y >= height)break;  /* 退出 */

        bsp_8080_lcd_show_char(x, y, *p, size, 0, color,bsp_8080_lcd_parameter.g_back_color);
        x += size / 2;
        p++;
    }
}

/**
 * 说明       LCD打印窗口初始化
 * 输入       x,y         : 起始坐标
 * 输入       x_end,y_end:  结束点坐标
 * 输入       size        : 选择字体 FONT_ASCII_12_6/FONT_ASCII_16_8/FONT_ASCII_24_12/FONT_ASCII_32_16
 * 输入       back_color  : 背景颜色
 * 输入       point_color   字体颜色
 * 返回值      无
 */
void bsp_8080_lcd_printf_init(uint16_t x_start,uint16_t y_start,uint16_t x_end,uint16_t y_end,FONT_ASCII char_size,uint16_t back_color,uint16_t point_color)
{
    bsp_8080_lcd_pritnf_parameter.x_start=x_start;
    bsp_8080_lcd_pritnf_parameter.y_start=y_start;

    bsp_8080_lcd_pritnf_parameter.x_end=x_end;
    bsp_8080_lcd_pritnf_parameter.y_end=y_end;

    bsp_8080_lcd_pritnf_parameter.x_now=x_start;
    bsp_8080_lcd_pritnf_parameter.y_now=y_start;

    bsp_8080_lcd_pritnf_parameter.size=char_size;

    bsp_8080_lcd_pritnf_parameter.back_color=back_color;   
    bsp_8080_lcd_pritnf_parameter.point_color=point_color; 

    bsp_8080_lcd_fill(bsp_8080_lcd_pritnf_parameter.x_start,bsp_8080_lcd_pritnf_parameter.y_start,bsp_8080_lcd_pritnf_parameter.x_end,bsp_8080_lcd_pritnf_parameter.y_end,bsp_8080_lcd_pritnf_parameter.back_color);    
}


/**
 * 说明       LCD打印
 * 输入       ...和printf相同用法，自动换行
 * 返回值      无
 */
void bsp_8080_lcd_printf(const char * sFormat, ...) 
{
    #define PRINTF_MAX_LEN 100
    char printf_buffer[PRINTF_MAX_LEN];
    char* p=printf_buffer;
    uint16_t len=0,count=0;
    va_list ParamList;
    va_start(ParamList, sFormat);     
    vsprintf(printf_buffer,sFormat, ParamList);     
    va_end(ParamList); 

    len=strlen(printf_buffer);    

    while( ( ((*p <= '~') && (*p >= ' ')) || (*p =='\r') || (*p =='\n') ) && (count<len) )   /* 判断是不是非法字符! */
    {
        if((*p =='\r'))
        {
            bsp_8080_lcd_pritnf_parameter.x_now = bsp_8080_lcd_pritnf_parameter.x_start;
        }        
        else if((*p =='\n'))
        {
            bsp_8080_lcd_pritnf_parameter.y_now += bsp_8080_lcd_pritnf_parameter.size; 
            bsp_8080_lcd_fill(bsp_8080_lcd_pritnf_parameter.x_now,bsp_8080_lcd_pritnf_parameter.y_now,bsp_8080_lcd_pritnf_parameter.x_end,bsp_8080_lcd_pritnf_parameter.y_now+bsp_8080_lcd_pritnf_parameter.size,bsp_8080_lcd_pritnf_parameter.back_color);            
        }
        else if( (bsp_8080_lcd_pritnf_parameter.x_now + bsp_8080_lcd_pritnf_parameter.size/2) > bsp_8080_lcd_pritnf_parameter.x_end)
        {
            bsp_8080_lcd_pritnf_parameter.x_now = bsp_8080_lcd_pritnf_parameter.x_start;
            bsp_8080_lcd_pritnf_parameter.y_now += bsp_8080_lcd_pritnf_parameter.size;
            bsp_8080_lcd_fill(bsp_8080_lcd_pritnf_parameter.x_now,bsp_8080_lcd_pritnf_parameter.y_now,bsp_8080_lcd_pritnf_parameter.x_end,bsp_8080_lcd_pritnf_parameter.y_now+bsp_8080_lcd_pritnf_parameter.size,bsp_8080_lcd_pritnf_parameter.back_color);                        
        }
        else if ( (bsp_8080_lcd_pritnf_parameter.y_now+bsp_8080_lcd_pritnf_parameter.size) > bsp_8080_lcd_pritnf_parameter.y_end)
        {
            bsp_8080_lcd_pritnf_parameter.x_now = bsp_8080_lcd_pritnf_parameter.x_start;
            bsp_8080_lcd_pritnf_parameter.y_now = bsp_8080_lcd_pritnf_parameter.y_start;            
            bsp_8080_lcd_fill(bsp_8080_lcd_pritnf_parameter.x_start,bsp_8080_lcd_pritnf_parameter.y_start,bsp_8080_lcd_pritnf_parameter.x_end,bsp_8080_lcd_pritnf_parameter.y_end,bsp_8080_lcd_pritnf_parameter.back_color);
        }

        if((*p !='\r')&&(*p !='\n'))
        {
            bsp_8080_lcd_show_char(bsp_8080_lcd_pritnf_parameter.x_now, bsp_8080_lcd_pritnf_parameter.y_now, *p, bsp_8080_lcd_pritnf_parameter.size, 0, bsp_8080_lcd_pritnf_parameter.point_color,bsp_8080_lcd_pritnf_parameter.back_color);
            bsp_8080_lcd_pritnf_parameter.x_now += bsp_8080_lcd_pritnf_parameter.size / 2;            
        }        
        p++;
        count++;
    }

}


void bsp_8080_lcd_show_log(void)
{
    //显示log图片
    bsp_8080_lcd_color_fill(60,0,233,99,(uint16_t*)gd_log_picture);
}



