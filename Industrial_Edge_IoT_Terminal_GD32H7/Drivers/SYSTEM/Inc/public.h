#ifndef __PUBLIC_H_
#define __PUBLIC_H_

#include "gd32h7xx.h"

#define LCD_DEBUG  1

//#define TFT_8080__LCD
//#define TFT_RGB_LCD

#if (LCD_DEBUG == 1)
    #ifdef TFT_RGB_LCD
        #include "bsp_rgb_lcd.h"
        #define printf_log(format, ...)   printf(format, ##__VA_ARGS__);bsp_rgb_lcd_printf(format, ##__VA_ARGS__);
    #else
        #include "bsp_8080_lcd.h"
        #define printf_log(format, ...)   printf(format, ##__VA_ARGS__);bsp_8080_lcd_printf(format, ##__VA_ARGS__);
    #endif   
#else
    #define printf_log(format, ...)   printf(format, ##__VA_ARGS__);
#endif                                  
    
extern volatile uint64_t driver_tick;

typedef enum {DRV_ERROR = 0, DRV_SUCCESS = !DRV_ERROR} Drv_Err;

typedef struct __typdef_tick_handle{
    uint16_t tick_value;
    void (*tick_task_callback)(void);  
}typdef_tick_handle ;

extern typdef_tick_handle driver_tick_handle[];

typedef union  _com_union{
    uint8_t Com_State ;
    struct {
        uint8_t RecSuccess: 1 ;
        uint8_t SendSuccess:1 ;
        uint8_t RecState:   1 ;
        uint8_t SendState:  1 ;
        uint8_t Dir:        1 ;
        uint8_t Unused:     4 ;
    }Bits ;
}com_union;

typedef struct __typdef_com_control_general{
    com_union Com_Flag ;
    uint8_t*  p_Rec;
    uint8_t*  p_Send;
    uint16_t  SendSize;
    uint16_t  SendCount;
    uint16_t  RecSize;    
    uint16_t  RecCount;     
}typdef_com_control_general ;

typedef enum
{
    MODE_POLL,     
    MODE_DMA,    
    MODE_INT,   
}com_mode_enum;

#include "driver_gpio.h"
#include "driver_dma.h"
#include "stdlib.h"
#include <stdio.h>
#include <string.h> 

/*宏定义*/
#define AT_ADDR(__ADDR)         __attribute__ ((section(".ARM.__at_" #__ADDR)))

#define SET_AREA(__AREA)        __attribute__((section(""#__AREA)))

#define SET_ADDR(__ADDR)        AT_ADDR(__ADDR)

/*变量/代码存储位置*/
#define SET_RAM_CODE            __attribute__ ((aligned(64)))   SET_AREA(RAM_CODE)              //存储至SRAM
#define SET_FLASH_AREA          __attribute__ ((aligned(64)))   SET_AREA(FLASH_AREA)              //存储至SRAM
#define SET_SDRAM               __attribute__ ((aligned(64)))   SET_AREA(SDRAM)               //存储至SRAM
#define SET_DMA_AREA_RAM        __attribute__ ((aligned(64)))    SET_AREA(DMA_AREA_RAM)    //DMA内部RAM快速区
#define SET_DMA_AREA_SDRAM      __attribute__ ((aligned(64)))    SET_AREA(DMA_AREA_SDRAM)  //DMA SDRAM低速区
#define SET_DTC                 __attribute__ ((aligned(64)))    SET_AREA(DTC_RAM)              //存储到DTC区
#define SET_AXI                 __attribute__ ((aligned(64)))    SET_AREA(AXI_RAM)              //存储到AXI区

#define SET_VECTOR_TABLE        __attribute__ ((aligned(0x400)))  SET_AREA(VECTOR_TABLE)  //DMA SDRAM低速区

void Delay_init(uint16_t sysclk);
Drv_Err delay_init(void);
// void driver_init(void);
void delay_us(uint32_t count);
void delay_ms(uint32_t count);
void driver_tic_inc(void);
void delay_sysclk(uint32_t count);
//void printf_log(const char * sFormat, ...);
Drv_Err memory_compare(uint8_t* src, uint8_t* dst, uint16_t length);
void cache_enable(void);
void cache_disable(void);

void mpu_config(uint32_t region_base_address,uint32_t region_size,uint32_t access_permission,\
                                    uint32_t access_bufferable,uint32_t access_cacheable,uint32_t access_shareable,\
                                    uint32_t instruction_exec,uint32_t tex_type);
void driver_system_config(void);
SET_FLASH_AREA void systick_cfg(void);

#endif /*#ifndef __PUBLIC_H_*/
