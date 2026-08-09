#ifndef BSP_8080_LCD_H
#define BSP_8080_LCD_H

#include "public.h"

#ifndef __BSP_LCD_FONT_H
    #include "lcd_font.h"
#endif


#ifndef __GD_LOG_H
    #include "gd_log.h"
#endif


#define EXMC_BANK0_NORSRAM_REGION_LCD(x) EXMC_BANK0_NORSRAM_REGION##x

#define EXMC_LCD(NEx) EXMC_BANK0_NORSRAM_REGION_LCD(NEx)

/* 字体大小枚举 */
typedef enum {LCD_MODE = 0,SRAM_MODE =1} DISPLAY_MODE;

/* LCD参数结构体 */
typedef struct __typde_8080_lcd_parameter_struct
{
    uint16_t width;     /* LCD 宽度 */
    uint16_t height;    /* LCD 高度 */
    uint16_t id;        /* LCD ID   */
    uint8_t  dir;       /* 横屏还是竖屏控制：0，竖屏；1，横屏 */
    uint16_t wramcmd;   /* 开始写gram指令 */
    uint16_t setxcmd;   /* 设置x坐标指令  */
    uint16_t setycmd;   /* 设置y坐标指令  */
    
    /* LCD的画笔颜色和背景色 */
    __IO uint32_t g_point_color;    /* 画笔颜色 */
    __IO uint32_t g_back_color;    /* 背景色 */  

    DISPLAY_MODE display_mode;    /* 显示模式 */ 
    
    uint16_t* display_ram;    /* 显示模式 */      
}typde_8080_lcd_parameter_struct;


/* LCD prinft参数结构体 */
typedef struct __typde_8080_lcd_printf_parameter_struct
{
    uint16_t x_start;    
    uint16_t y_start;    
    uint16_t x_end;    
    uint16_t  y_end;    
    uint16_t x_now;  
    uint16_t y_now;  
    
    FONT_ASCII size; 

    /* LCD的画笔颜色和背景色 */
    __IO uint32_t point_color;    /* 画笔颜色 */
    __IO uint32_t back_color;    /* 背景色 */     
}typde_8080_lcd_printf_parameter_struct;
/******************************************************************************************/
/* LCD参数 */
extern typde_8080_lcd_parameter_struct bsp_8080_lcd_parameter; /* 管理LCD重要参数 */

extern typde_8080_lcd_printf_parameter_struct bsp_8080_lcd_pritnf_parameter;

/******************************************************************************************/
/* LCD扫描方向和颜色 定义 */

/* 扫描方向定义 */
#define L2R_U2D         0           /* 从左到右,从上到下 */
#define L2R_D2U         1           /* 从左到右,从下到上 */
#define R2L_U2D         2           /* 从右到左,从上到下 */
#define R2L_D2U         3           /* 从右到左,从下到上 */

#define U2D_L2R         4           /* 从上到下,从左到右 */
#define U2D_R2L         5           /* 从上到下,从右到左 */
#define D2U_L2R         6           /* 从下到上,从左到右 */
#define D2U_R2L         7           /* 从下到上,从右到左 */

#define DFT_SCAN_DIR    L2R_U2D     /* 默认的扫描方向 */

/* 常用画笔颜色 */
#define WHITE           0xFFFF      /* 白色 */
#define BLACK           0x0000      /* 黑色 */
#define RED             0xF800      /* 红色 */
#define GREEN           0x07E0      /* 绿色 */
#define BLUE            0x001F      /* 蓝色 */ 
#define MAGENTA         0XF81F      /* 品红色/紫红色 = BLUE + RED */
#define YELLOW          0XFFE0      /* 黄色 = GREEN + RED */
#define CYAN            0X07FF      /* 青色 = GREEN + BLUE */  

/* 非常用颜色 */
#define BROWN           0XBC40      /* 棕色 */
#define BRRED           0XFC07      /* 棕红色 */
#define GRAY            0X8430      /* 灰色 */ 
#define DARKBLUE        0X01CF      /* 深蓝色 */
#define LIGHTBLUE       0X7D7C      /* 浅蓝色 */ 
#define GRAYBLUE        0X5458      /* 灰蓝色 */ 
#define LIGHTGREEN      0X841F      /* 浅绿色 */  
#define LGRAY           0XC618      /* 浅灰色(PANNEL),窗体背景色 */ 
#define LGRAYBLUE       0XA651      /* 浅灰蓝色(中间层颜色) */ 
#define LBBLUE          0X2B12      /* 浅棕蓝色(选择条目的反色) */ 

/******************************************************************************************/

  
/******************************************************************************************/
/* 函数申明 */

void bsp_8080_lcd_backlight_off(void);
void bsp_8080_lcd_backlight_on(void);
void bsp_8080_lcd_backlight_duty_set(uint8_t backlight_value);


void bsp_8080_lcd_wr_data(volatile uint16_t data);            /* LCD写数据 */
void bsp_8080_lcd_wr_regno(volatile uint16_t regno);          /* LCD写寄存器编号/地址 */
void bsp_8080_lcd_write_reg(uint16_t regno, uint16_t data);   /* LCD写寄存器的值 */


uint32_t bsp_8080_lcd_init(void);                        /* 初始化LCD */ 
void bsp_8080_lcd_display_on(void);                  /* 开显示 */ 
void bsp_8080_lcd_display_off(void);                 /* 关显示 */
void bsp_8080_lcd_scan_dir(uint8_t dir);             /* 设置屏扫描方向 */ 
void bsp_8080_lcd_display_dir(uint8_t dir);          /* 设置屏幕显示方向 */ 
void bsp_8080_lcd_ssd_backlight_set(uint8_t pwm);    /* SSD1963 背光控制 */ 

void bsp_8080_lcd_set_cursor(uint16_t x, uint16_t y);    /* 设置光标 */ 
uint32_t bsp_8080_lcd_read_point(uint16_t x, uint16_t y);/* 读点(32位颜色,兼容LTDC)  */
void bsp_8080_lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);/* 画点(32位颜色,兼容LTDC) */

void bsp_8080_lcd_clear(uint16_t color);     /* LCD清屏 */
void bsp_8080_lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);                   /* 填充实心圆 */
void bsp_8080_lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);                  /* 画圆 */
void bsp_8080_lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color);                  /* 画水平线 */
void bsp_8080_lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);             /* 设置窗口 */
void bsp_8080_lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);          /* 纯色填充矩形(32位颜色,兼容LTDC) */
void bsp_8080_lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);   /* 彩色填充矩形 */
void bsp_8080_lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);     /* 画直线 */
void bsp_8080_lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);/* 画矩形 */


void bsp_8080_lcd_show_char(uint16_t x, uint16_t y, char chr, FONT_ASCII size, uint8_t mode, uint16_t color,uint16_t back_color);
void bsp_8080_lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, FONT_ASCII size, uint16_t color);
void bsp_8080_lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, FONT_ASCII size, uint8_t mode, uint16_t color);
void bsp_8080_lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, FONT_ASCII size, char *p, uint16_t color);


void bsp_8080_lcd_printf_init(uint16_t x_start,uint16_t y_start,uint16_t x_end,uint16_t y_end,FONT_ASCII char_size,uint16_t back_color,uint16_t point_color);
void bsp_8080_lcd_printf(const char * sFormat, ...);
void bsp_8080_lcd_show_log(void);

void bsp_8080_lcd_mode_config(DISPLAY_MODE display_mode,uint16_t* display_ram);


void bsp_8080_lcd_sram_mode_updata(void);

        
#endif /* BSP_OLED_H */
