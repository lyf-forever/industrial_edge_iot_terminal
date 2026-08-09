#include "bsp_8080_lcd_init.h"


/**
 * @brief       ST7789 寄存器初始化代码
 * @param       无
 * @retval      无
 */
void lcd_ex_st7789_reginit(void)
{
    bsp_8080_lcd_wr_regno(0x11);

    delay_ms(120); 

    bsp_8080_lcd_wr_regno(0x36);
    bsp_8080_lcd_wr_data(0x00);


    bsp_8080_lcd_wr_regno(0x3A);
    bsp_8080_lcd_wr_data(0X05);

    bsp_8080_lcd_wr_regno(0xB2);
    bsp_8080_lcd_wr_data(0x0C);
    bsp_8080_lcd_wr_data(0x0C);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x33);
    bsp_8080_lcd_wr_data(0x33);

    bsp_8080_lcd_wr_regno(0xB7);
    bsp_8080_lcd_wr_data(0x35);

    bsp_8080_lcd_wr_regno(0xBB); /* vcom */
    bsp_8080_lcd_wr_data(0x32);  /* 30 */

    bsp_8080_lcd_wr_regno(0xC0);
    bsp_8080_lcd_wr_data(0x0C);

    bsp_8080_lcd_wr_regno(0xC2);
    bsp_8080_lcd_wr_data(0x01);

    bsp_8080_lcd_wr_regno(0xC3); /* vrh */
    bsp_8080_lcd_wr_data(0x10);  /* 17 0D */

    bsp_8080_lcd_wr_regno(0xC4); /* vdv */
    bsp_8080_lcd_wr_data(0x20);  /* 20 */

    bsp_8080_lcd_wr_regno(0xC6);
    bsp_8080_lcd_wr_data(0x0f);

    bsp_8080_lcd_wr_regno(0xD0);
    bsp_8080_lcd_wr_data(0xA4); 
    bsp_8080_lcd_wr_data(0xA1); 

    bsp_8080_lcd_wr_regno(0xE0); /* Set Gamma  */
    bsp_8080_lcd_wr_data(0xd0);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x02);
    bsp_8080_lcd_wr_data(0x07);
    bsp_8080_lcd_wr_data(0x0a);
    bsp_8080_lcd_wr_data(0x28);
    bsp_8080_lcd_wr_data(0x32);
    bsp_8080_lcd_wr_data(0X44);
    bsp_8080_lcd_wr_data(0x42);
    bsp_8080_lcd_wr_data(0x06);
    bsp_8080_lcd_wr_data(0x0e);
    bsp_8080_lcd_wr_data(0x12);
    bsp_8080_lcd_wr_data(0x14);
    bsp_8080_lcd_wr_data(0x17);


    bsp_8080_lcd_wr_regno(0XE1);  /* Set Gamma */
    bsp_8080_lcd_wr_data(0xd0);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x02);
    bsp_8080_lcd_wr_data(0x07);
    bsp_8080_lcd_wr_data(0x0a);
    bsp_8080_lcd_wr_data(0x28);
    bsp_8080_lcd_wr_data(0x31);
    bsp_8080_lcd_wr_data(0x54);
    bsp_8080_lcd_wr_data(0x47);
    bsp_8080_lcd_wr_data(0x0e);
    bsp_8080_lcd_wr_data(0x1c);
    bsp_8080_lcd_wr_data(0x17);
    bsp_8080_lcd_wr_data(0x1b); 
    bsp_8080_lcd_wr_data(0x1e);


    bsp_8080_lcd_wr_regno(0x2A);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xef);

    bsp_8080_lcd_wr_regno(0x2B);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x01);
    bsp_8080_lcd_wr_data(0x3f);

    bsp_8080_lcd_wr_regno(0x29); /* display on */
}

/**
 * @brief       ILI9488寄存器初始化代码
 * @param       无
 * @retval      无
 */
void lcd_ex_ili9488_reginit(void)
{
    //************* Start Initial Sequence **********//
    bsp_8080_lcd_wr_regno(0XF7);    	
    bsp_8080_lcd_wr_data(0xA9); 	
    bsp_8080_lcd_wr_data(0x51); 	
    bsp_8080_lcd_wr_data(0x2C); 	
    bsp_8080_lcd_wr_data(0x82);

    bsp_8080_lcd_wr_regno(0XEC);    	
    bsp_8080_lcd_wr_data(0x00); 	
    bsp_8080_lcd_wr_data(0x02); 	
    bsp_8080_lcd_wr_data(0x03); 	
    bsp_8080_lcd_wr_data(0x7A);

    bsp_8080_lcd_wr_regno(0xC0); 	
    bsp_8080_lcd_wr_data(0x13); 	
    bsp_8080_lcd_wr_data(0x13); 	
        
    bsp_8080_lcd_wr_regno(0xC1); 	
    bsp_8080_lcd_wr_data(0x41); 	
        
    bsp_8080_lcd_wr_regno(0xC5); 	
    bsp_8080_lcd_wr_data(0x00); 	
    bsp_8080_lcd_wr_data(0x28); 	
    bsp_8080_lcd_wr_data(0x80);
        
    bsp_8080_lcd_wr_regno(0xB1);   //Frame rate 70HZ  	
    bsp_8080_lcd_wr_data(0xB0);
    bsp_8080_lcd_wr_data(0x11);	
        
    bsp_8080_lcd_wr_regno(0xB4); 	
    bsp_8080_lcd_wr_data(0x02);   	
        
    bsp_8080_lcd_wr_regno(0xB6); //RGB/MCU Interface Control	
    bsp_8080_lcd_wr_data(0x02);   //MCU	
    bsp_8080_lcd_wr_data(0x22); 

    bsp_8080_lcd_wr_regno(0xB7); 	
    bsp_8080_lcd_wr_data(0xc6); 

    bsp_8080_lcd_wr_regno(0xBE); 	
    bsp_8080_lcd_wr_data(0x00); 
    bsp_8080_lcd_wr_data(0x04);	
        
    bsp_8080_lcd_wr_regno(0xE9); 	
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xF4); 	
    bsp_8080_lcd_wr_data(0x00); 
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x0f);	
        
    bsp_8080_lcd_wr_regno(0xE0); 	
    bsp_8080_lcd_wr_data(0x00); 	
    bsp_8080_lcd_wr_data(0x04); 	
    bsp_8080_lcd_wr_data(0x0E); 	
    bsp_8080_lcd_wr_data(0x08); 	
    bsp_8080_lcd_wr_data(0x17); 	
    bsp_8080_lcd_wr_data(0x0A); 	
    bsp_8080_lcd_wr_data(0x40); 	
    bsp_8080_lcd_wr_data(0x79); 	
    bsp_8080_lcd_wr_data(0x4D); 	
    bsp_8080_lcd_wr_data(0x07); 	
    bsp_8080_lcd_wr_data(0x0E); 	
    bsp_8080_lcd_wr_data(0x0A); 	
    bsp_8080_lcd_wr_data(0x1A); 	
    bsp_8080_lcd_wr_data(0x1D); 	
    bsp_8080_lcd_wr_data(0x0F);  	
        
    bsp_8080_lcd_wr_regno(0xE1); 	
    bsp_8080_lcd_wr_data(0x00); 	
    bsp_8080_lcd_wr_data(0x1B); 	
    bsp_8080_lcd_wr_data(0x1F); 	
    bsp_8080_lcd_wr_data(0x02); 	
    bsp_8080_lcd_wr_data(0x10); 	
    bsp_8080_lcd_wr_data(0x05); 	
    bsp_8080_lcd_wr_data(0x32); 	
    bsp_8080_lcd_wr_data(0x34); 	
    bsp_8080_lcd_wr_data(0x43); 	
    bsp_8080_lcd_wr_data(0x02); 	
    bsp_8080_lcd_wr_data(0x0A); 	
    bsp_8080_lcd_wr_data(0x09); 	
    bsp_8080_lcd_wr_data(0x33); 	
    bsp_8080_lcd_wr_data(0x37); 	
    bsp_8080_lcd_wr_data(0x0F); 


    bsp_8080_lcd_wr_regno(0xF4);      
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x0f);	
        
    bsp_8080_lcd_wr_regno(0x36); 	
    bsp_8080_lcd_wr_data(0x08); 	
        
    bsp_8080_lcd_wr_regno(0x3A);  //Interface Mode Control	
    bsp_8080_lcd_wr_data(0x55);  //0x66 18bit; 0x55 16bit			
        
    bsp_8080_lcd_wr_regno(0x20); 	
    bsp_8080_lcd_wr_regno(0x11); 	
    delay_ms(120); 	
    bsp_8080_lcd_wr_regno(0x29); 	
    delay_ms(50); 

} 

/**
 * @brief       ILI9341寄存器初始化代码
 * @param       无
 * @retval      无
 */
void lcd_ex_ili9341_reginit(void)
{
    bsp_8080_lcd_wr_regno(0xCF);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xC1);
    bsp_8080_lcd_wr_data(0X30);
    bsp_8080_lcd_wr_regno(0xED);
    bsp_8080_lcd_wr_data(0x64);
    bsp_8080_lcd_wr_data(0x03);
    bsp_8080_lcd_wr_data(0X12);
    bsp_8080_lcd_wr_data(0X81);
    bsp_8080_lcd_wr_regno(0xE8);
    bsp_8080_lcd_wr_data(0x85);
    bsp_8080_lcd_wr_data(0x10);
    bsp_8080_lcd_wr_data(0x7A);
    bsp_8080_lcd_wr_regno(0xCB);
    bsp_8080_lcd_wr_data(0x39);
    bsp_8080_lcd_wr_data(0x2C);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x34);
    bsp_8080_lcd_wr_data(0x02);
    bsp_8080_lcd_wr_regno(0xF7);
    bsp_8080_lcd_wr_data(0x20);
    bsp_8080_lcd_wr_regno(0xEA);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_regno(0xC0); /* Power control */
    bsp_8080_lcd_wr_data(0x1B);  /* VRH[5:0] */
    bsp_8080_lcd_wr_regno(0xC1); /* Power control */
    bsp_8080_lcd_wr_data(0x01);  /* SAP[2:0];BT[3:0] */
    bsp_8080_lcd_wr_regno(0xC5); /* VCM control */
    bsp_8080_lcd_wr_data(0x30);  /* 3F */
    bsp_8080_lcd_wr_data(0x30);  /* 3C */
    bsp_8080_lcd_wr_regno(0xC7); /* VCM control2 */
    bsp_8080_lcd_wr_data(0XB7);
    bsp_8080_lcd_wr_regno(0x36); /*  Memory Access Control */
    bsp_8080_lcd_wr_data(0x48);
    bsp_8080_lcd_wr_regno(0x3A);
    bsp_8080_lcd_wr_data(0x55);
    bsp_8080_lcd_wr_regno(0xB1);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x1A);
    bsp_8080_lcd_wr_regno(0xB6); /*  Display Function Control */
    bsp_8080_lcd_wr_data(0x0A);
    bsp_8080_lcd_wr_data(0xA2);
    bsp_8080_lcd_wr_regno(0xF2); /*  3Gamma Function Disable */
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_regno(0x26); /* Gamma curve selected */
    bsp_8080_lcd_wr_data(0x01);
    bsp_8080_lcd_wr_regno(0xE0); /* Set Gamma */
    bsp_8080_lcd_wr_data(0x0F);
    bsp_8080_lcd_wr_data(0x2A);
    bsp_8080_lcd_wr_data(0x28);
    bsp_8080_lcd_wr_data(0x08);
    bsp_8080_lcd_wr_data(0x0E);
    bsp_8080_lcd_wr_data(0x08);
    bsp_8080_lcd_wr_data(0x54);
    bsp_8080_lcd_wr_data(0XA9);
    bsp_8080_lcd_wr_data(0x43);
    bsp_8080_lcd_wr_data(0x0A);
    bsp_8080_lcd_wr_data(0x0F);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_regno(0XE1);    /* Set Gamma */
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x15);
    bsp_8080_lcd_wr_data(0x17);
    bsp_8080_lcd_wr_data(0x07);
    bsp_8080_lcd_wr_data(0x11);
    bsp_8080_lcd_wr_data(0x06);
    bsp_8080_lcd_wr_data(0x2B);
    bsp_8080_lcd_wr_data(0x56);
    bsp_8080_lcd_wr_data(0x3C);
    bsp_8080_lcd_wr_data(0x05);
    bsp_8080_lcd_wr_data(0x10);
    bsp_8080_lcd_wr_data(0x0F);
    bsp_8080_lcd_wr_data(0x3F);
    bsp_8080_lcd_wr_data(0x3F);
    bsp_8080_lcd_wr_data(0x0F);
    bsp_8080_lcd_wr_regno(0x2B);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x01);
    bsp_8080_lcd_wr_data(0x3f);
    bsp_8080_lcd_wr_regno(0x2A);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xef);
    bsp_8080_lcd_wr_regno(0x11); /* Exit Sleep */
    delay_ms(120);
    bsp_8080_lcd_wr_regno(0x29); /* display on */
 }
 

/**
 * @brief       NT35310寄存器初始化代码 
 * @param       无
 * @retval      无
 */
void lcd_ex_nt35310_reginit(void)
{
    bsp_8080_lcd_wr_regno(0xED);
    bsp_8080_lcd_wr_data(0x01);
    bsp_8080_lcd_wr_data(0xFE);

    bsp_8080_lcd_wr_regno(0xEE);
    bsp_8080_lcd_wr_data(0xDE);
    bsp_8080_lcd_wr_data(0x21);

    bsp_8080_lcd_wr_regno(0xF1);
    bsp_8080_lcd_wr_data(0x01);
    bsp_8080_lcd_wr_regno(0xDF);
    bsp_8080_lcd_wr_data(0x10);

    /* VCOMvoltage */
    bsp_8080_lcd_wr_regno(0xC4);
    bsp_8080_lcd_wr_data(0x8F);  /* 5f */

    bsp_8080_lcd_wr_regno(0xC6);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xE2);
    bsp_8080_lcd_wr_data(0xE2);
    bsp_8080_lcd_wr_data(0xE2);
    bsp_8080_lcd_wr_regno(0xBF);
    bsp_8080_lcd_wr_data(0xAA);

    bsp_8080_lcd_wr_regno(0xB0);
    bsp_8080_lcd_wr_data(0x0D);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x0D);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x11);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x19);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x21);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x2D);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x3D);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x5D);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x5D);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xB1);
    bsp_8080_lcd_wr_data(0x80);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x8B);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x96);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xB2);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x02);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x03);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xB3);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xB4);
    bsp_8080_lcd_wr_data(0x8B);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x96);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xA1);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xB5);
    bsp_8080_lcd_wr_data(0x02);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x03);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x04);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xB6);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xB7);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x3F);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x5E);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x64);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x8C);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xAC);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xDC);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x70);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x90);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xEB);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xDC);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xB8);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xBA);
    bsp_8080_lcd_wr_data(0x24);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xC1);
    bsp_8080_lcd_wr_data(0x20);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x54);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xFF);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xC2);
    bsp_8080_lcd_wr_data(0x0A);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x04);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xC3);
    bsp_8080_lcd_wr_data(0x3C);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x3A);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x39);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x37);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x3C);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x36);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x32);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x2F);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x2C);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x29);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x26);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x24);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x24);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x23);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x3C);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x36);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x32);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x2F);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x2C);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x29);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x26);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x24);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x24);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x23);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xC4);
    bsp_8080_lcd_wr_data(0x62);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x05);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x84);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xF0);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x18);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xA4);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x18);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x50);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x0C);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x17);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x95);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xF3);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xE6);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xC5);
    bsp_8080_lcd_wr_data(0x32);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x44);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x65);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x76);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x88);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xC6);
    bsp_8080_lcd_wr_data(0x20);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x17);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x01);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xC7);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xC8);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xC9);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xE0);
    bsp_8080_lcd_wr_data(0x16);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x1C);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x21);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x36);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x46);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x52);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x64);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x7A);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x8B);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x99);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xA8);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xB9);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xC4);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xCA);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xD2);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xD9);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xE0);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xF3);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xE1);
    bsp_8080_lcd_wr_data(0x16);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x1C);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x22);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x36);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x45);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x52);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x64);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x7A);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x8B);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x99);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xA8);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xB9);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xC4);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xCA);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xD2);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xD8);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xE0);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xF3);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xE2);
    bsp_8080_lcd_wr_data(0x05);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x0B);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x1B);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x34);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x44);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x4F);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x61);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x79);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x88);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x97);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xA6);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xB7);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xC2);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xC7);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xD1);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xD6);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xDD);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xF3);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_regno(0xE3);
    bsp_8080_lcd_wr_data(0x05);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xA);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x1C);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x33);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x44);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x50);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x62);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x78);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x88);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x97);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xA6);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xB7);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xC2);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xC7);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xD1);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xD5);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xDD);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xF3);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xE4);
    bsp_8080_lcd_wr_data(0x01);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x01);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x02);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x2A);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x3C);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x4B);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x5D);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x74);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x84);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x93);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xA2);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xB3);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xBE);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xC4);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xCD);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xD3);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xDD);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xF3);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_regno(0xE5);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x02);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x29);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x3C);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x4B);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x5D);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x74);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x84);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x93);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xA2);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xB3);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xBE);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xC4);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xCD);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xD3);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xDC);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xF3);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xE6);
    bsp_8080_lcd_wr_data(0x11);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x34);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x56);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x76);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x77);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x66);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x88);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x99);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xBB);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x99);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x66);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x55);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x55);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x45);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x43);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x44);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xE7);
    bsp_8080_lcd_wr_data(0x32);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x55);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x76);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x66);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x67);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x67);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x87);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x99);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xBB);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x99);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x77);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x44);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x56);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x23);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x33);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x45);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xE8);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x99);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x87);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x88);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x77);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x66);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x88);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xAA);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0xBB);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x99);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x66);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x55);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x55);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x44);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x44);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x55);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xE9);
    bsp_8080_lcd_wr_data(0xAA);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0x00);
    bsp_8080_lcd_wr_data(0xAA);

    bsp_8080_lcd_wr_regno(0xCF);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xF0);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x50);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xF3);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0xF9);
    bsp_8080_lcd_wr_data(0x06);
    bsp_8080_lcd_wr_data(0x10);
    bsp_8080_lcd_wr_data(0x29);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0x3A);
    bsp_8080_lcd_wr_data(0x55);  /* 66 */

    bsp_8080_lcd_wr_regno(0x11);
    delay_ms(100);
    bsp_8080_lcd_wr_regno(0x29);
    bsp_8080_lcd_wr_regno(0x35);
    bsp_8080_lcd_wr_data(0x00);

    bsp_8080_lcd_wr_regno(0x51);
    bsp_8080_lcd_wr_data(0xFF);
    bsp_8080_lcd_wr_regno(0x53);
    bsp_8080_lcd_wr_data(0x2C);
    bsp_8080_lcd_wr_regno(0x55);
    bsp_8080_lcd_wr_data(0x82);
    bsp_8080_lcd_wr_regno(0x2c);
}

/**
 * @brief       NT35510寄存器初始化代码 
 * @param       无
 * @retval      无
 */
void lcd_ex_nt35510_reginit(void)
{
    bsp_8080_lcd_write_reg(0xF000, 0x55);
    bsp_8080_lcd_write_reg(0xF001, 0xAA);
    bsp_8080_lcd_write_reg(0xF002, 0x52);
    bsp_8080_lcd_write_reg(0xF003, 0x08);
    bsp_8080_lcd_write_reg(0xF004, 0x01);
    /* AVDD Set AVDD 5.2V */
    bsp_8080_lcd_write_reg(0xB000, 0x0D);
    bsp_8080_lcd_write_reg(0xB001, 0x0D);
    bsp_8080_lcd_write_reg(0xB002, 0x0D);
    /* AVDD ratio */
    bsp_8080_lcd_write_reg(0xB600, 0x34);
    bsp_8080_lcd_write_reg(0xB601, 0x34);
    bsp_8080_lcd_write_reg(0xB602, 0x34);
    /* AVEE -5.2V */
    bsp_8080_lcd_write_reg(0xB100, 0x0D);
    bsp_8080_lcd_write_reg(0xB101, 0x0D);
    bsp_8080_lcd_write_reg(0xB102, 0x0D);
    /* AVEE ratio */
    bsp_8080_lcd_write_reg(0xB700, 0x34);
    bsp_8080_lcd_write_reg(0xB701, 0x34);
    bsp_8080_lcd_write_reg(0xB702, 0x34);
    /* VCL -2.5V */
    bsp_8080_lcd_write_reg(0xB200, 0x00);
    bsp_8080_lcd_write_reg(0xB201, 0x00);
    bsp_8080_lcd_write_reg(0xB202, 0x00);
    /* VCL ratio */
    bsp_8080_lcd_write_reg(0xB800, 0x24);
    bsp_8080_lcd_write_reg(0xB801, 0x24);
    bsp_8080_lcd_write_reg(0xB802, 0x24);
    /* VGH 15V (Free pump) */
    bsp_8080_lcd_write_reg(0xBF00, 0x01);
    bsp_8080_lcd_write_reg(0xB300, 0x0F);
    bsp_8080_lcd_write_reg(0xB301, 0x0F);
    bsp_8080_lcd_write_reg(0xB302, 0x0F);
    /* VGH ratio */
    bsp_8080_lcd_write_reg(0xB900, 0x34);
    bsp_8080_lcd_write_reg(0xB901, 0x34);
    bsp_8080_lcd_write_reg(0xB902, 0x34);
    /* VGL_REG -10V */
    bsp_8080_lcd_write_reg(0xB500, 0x08);
    bsp_8080_lcd_write_reg(0xB501, 0x08);
    bsp_8080_lcd_write_reg(0xB502, 0x08);
    bsp_8080_lcd_write_reg(0xC200, 0x03);
    /* VGLX ratio */
    bsp_8080_lcd_write_reg(0xBA00, 0x24);
    bsp_8080_lcd_write_reg(0xBA01, 0x24);
    bsp_8080_lcd_write_reg(0xBA02, 0x24);
    /* VGMP/VGSP 4.5V/0V */
    bsp_8080_lcd_write_reg(0xBC00, 0x00);
    bsp_8080_lcd_write_reg(0xBC01, 0x78);
    bsp_8080_lcd_write_reg(0xBC02, 0x00);
    /* VGMN/VGSN -4.5V/0V */
    bsp_8080_lcd_write_reg(0xBD00, 0x00);
    bsp_8080_lcd_write_reg(0xBD01, 0x78);
    bsp_8080_lcd_write_reg(0xBD02, 0x00);
    /* VCOM */
    bsp_8080_lcd_write_reg(0xBE00, 0x00);
    bsp_8080_lcd_write_reg(0xBE01, 0x64);
    /* Gamma Setting */
    bsp_8080_lcd_write_reg(0xD100, 0x00);
    bsp_8080_lcd_write_reg(0xD101, 0x33);
    bsp_8080_lcd_write_reg(0xD102, 0x00);
    bsp_8080_lcd_write_reg(0xD103, 0x34);
    bsp_8080_lcd_write_reg(0xD104, 0x00);
    bsp_8080_lcd_write_reg(0xD105, 0x3A);
    bsp_8080_lcd_write_reg(0xD106, 0x00);
    bsp_8080_lcd_write_reg(0xD107, 0x4A);
    bsp_8080_lcd_write_reg(0xD108, 0x00);
    bsp_8080_lcd_write_reg(0xD109, 0x5C);
    bsp_8080_lcd_write_reg(0xD10A, 0x00);
    bsp_8080_lcd_write_reg(0xD10B, 0x81);
    bsp_8080_lcd_write_reg(0xD10C, 0x00);
    bsp_8080_lcd_write_reg(0xD10D, 0xA6);
    bsp_8080_lcd_write_reg(0xD10E, 0x00);
    bsp_8080_lcd_write_reg(0xD10F, 0xE5);
    bsp_8080_lcd_write_reg(0xD110, 0x01);
    bsp_8080_lcd_write_reg(0xD111, 0x13);
    bsp_8080_lcd_write_reg(0xD112, 0x01);
    bsp_8080_lcd_write_reg(0xD113, 0x54);
    bsp_8080_lcd_write_reg(0xD114, 0x01);
    bsp_8080_lcd_write_reg(0xD115, 0x82);
    bsp_8080_lcd_write_reg(0xD116, 0x01);
    bsp_8080_lcd_write_reg(0xD117, 0xCA);
    bsp_8080_lcd_write_reg(0xD118, 0x02);
    bsp_8080_lcd_write_reg(0xD119, 0x00);
    bsp_8080_lcd_write_reg(0xD11A, 0x02);
    bsp_8080_lcd_write_reg(0xD11B, 0x01);
    bsp_8080_lcd_write_reg(0xD11C, 0x02);
    bsp_8080_lcd_write_reg(0xD11D, 0x34);
    bsp_8080_lcd_write_reg(0xD11E, 0x02);
    bsp_8080_lcd_write_reg(0xD11F, 0x67);
    bsp_8080_lcd_write_reg(0xD120, 0x02);
    bsp_8080_lcd_write_reg(0xD121, 0x84);
    bsp_8080_lcd_write_reg(0xD122, 0x02);
    bsp_8080_lcd_write_reg(0xD123, 0xA4);
    bsp_8080_lcd_write_reg(0xD124, 0x02);
    bsp_8080_lcd_write_reg(0xD125, 0xB7);
    bsp_8080_lcd_write_reg(0xD126, 0x02);
    bsp_8080_lcd_write_reg(0xD127, 0xCF);
    bsp_8080_lcd_write_reg(0xD128, 0x02);
    bsp_8080_lcd_write_reg(0xD129, 0xDE);
    bsp_8080_lcd_write_reg(0xD12A, 0x02);
    bsp_8080_lcd_write_reg(0xD12B, 0xF2);
    bsp_8080_lcd_write_reg(0xD12C, 0x02);
    bsp_8080_lcd_write_reg(0xD12D, 0xFE);
    bsp_8080_lcd_write_reg(0xD12E, 0x03);
    bsp_8080_lcd_write_reg(0xD12F, 0x10);
    bsp_8080_lcd_write_reg(0xD130, 0x03);
    bsp_8080_lcd_write_reg(0xD131, 0x33);
    bsp_8080_lcd_write_reg(0xD132, 0x03);
    bsp_8080_lcd_write_reg(0xD133, 0x6D);
    bsp_8080_lcd_write_reg(0xD200, 0x00);
    bsp_8080_lcd_write_reg(0xD201, 0x33);
    bsp_8080_lcd_write_reg(0xD202, 0x00);
    bsp_8080_lcd_write_reg(0xD203, 0x34);
    bsp_8080_lcd_write_reg(0xD204, 0x00);
    bsp_8080_lcd_write_reg(0xD205, 0x3A);
    bsp_8080_lcd_write_reg(0xD206, 0x00);
    bsp_8080_lcd_write_reg(0xD207, 0x4A);
    bsp_8080_lcd_write_reg(0xD208, 0x00);
    bsp_8080_lcd_write_reg(0xD209, 0x5C);
    bsp_8080_lcd_write_reg(0xD20A, 0x00);

    bsp_8080_lcd_write_reg(0xD20B, 0x81);
    bsp_8080_lcd_write_reg(0xD20C, 0x00);
    bsp_8080_lcd_write_reg(0xD20D, 0xA6);
    bsp_8080_lcd_write_reg(0xD20E, 0x00);
    bsp_8080_lcd_write_reg(0xD20F, 0xE5);
    bsp_8080_lcd_write_reg(0xD210, 0x01);
    bsp_8080_lcd_write_reg(0xD211, 0x13);
    bsp_8080_lcd_write_reg(0xD212, 0x01);
    bsp_8080_lcd_write_reg(0xD213, 0x54);
    bsp_8080_lcd_write_reg(0xD214, 0x01);
    bsp_8080_lcd_write_reg(0xD215, 0x82);
    bsp_8080_lcd_write_reg(0xD216, 0x01);
    bsp_8080_lcd_write_reg(0xD217, 0xCA);
    bsp_8080_lcd_write_reg(0xD218, 0x02);
    bsp_8080_lcd_write_reg(0xD219, 0x00);
    bsp_8080_lcd_write_reg(0xD21A, 0x02);
    bsp_8080_lcd_write_reg(0xD21B, 0x01);
    bsp_8080_lcd_write_reg(0xD21C, 0x02);
    bsp_8080_lcd_write_reg(0xD21D, 0x34);
    bsp_8080_lcd_write_reg(0xD21E, 0x02);
    bsp_8080_lcd_write_reg(0xD21F, 0x67);
    bsp_8080_lcd_write_reg(0xD220, 0x02);
    bsp_8080_lcd_write_reg(0xD221, 0x84);
    bsp_8080_lcd_write_reg(0xD222, 0x02);
    bsp_8080_lcd_write_reg(0xD223, 0xA4);
    bsp_8080_lcd_write_reg(0xD224, 0x02);
    bsp_8080_lcd_write_reg(0xD225, 0xB7);
    bsp_8080_lcd_write_reg(0xD226, 0x02);
    bsp_8080_lcd_write_reg(0xD227, 0xCF);
    bsp_8080_lcd_write_reg(0xD228, 0x02);
    bsp_8080_lcd_write_reg(0xD229, 0xDE);
    bsp_8080_lcd_write_reg(0xD22A, 0x02);
    bsp_8080_lcd_write_reg(0xD22B, 0xF2);
    bsp_8080_lcd_write_reg(0xD22C, 0x02);
    bsp_8080_lcd_write_reg(0xD22D, 0xFE);
    bsp_8080_lcd_write_reg(0xD22E, 0x03);
    bsp_8080_lcd_write_reg(0xD22F, 0x10);
    bsp_8080_lcd_write_reg(0xD230, 0x03);
    bsp_8080_lcd_write_reg(0xD231, 0x33);
    bsp_8080_lcd_write_reg(0xD232, 0x03);
    bsp_8080_lcd_write_reg(0xD233, 0x6D);
    bsp_8080_lcd_write_reg(0xD300, 0x00);
    bsp_8080_lcd_write_reg(0xD301, 0x33);
    bsp_8080_lcd_write_reg(0xD302, 0x00);
    bsp_8080_lcd_write_reg(0xD303, 0x34);
    bsp_8080_lcd_write_reg(0xD304, 0x00);
    bsp_8080_lcd_write_reg(0xD305, 0x3A);
    bsp_8080_lcd_write_reg(0xD306, 0x00);
    bsp_8080_lcd_write_reg(0xD307, 0x4A);
    bsp_8080_lcd_write_reg(0xD308, 0x00);
    bsp_8080_lcd_write_reg(0xD309, 0x5C);
    bsp_8080_lcd_write_reg(0xD30A, 0x00);

    bsp_8080_lcd_write_reg(0xD30B, 0x81);
    bsp_8080_lcd_write_reg(0xD30C, 0x00);
    bsp_8080_lcd_write_reg(0xD30D, 0xA6);
    bsp_8080_lcd_write_reg(0xD30E, 0x00);
    bsp_8080_lcd_write_reg(0xD30F, 0xE5);
    bsp_8080_lcd_write_reg(0xD310, 0x01);
    bsp_8080_lcd_write_reg(0xD311, 0x13);
    bsp_8080_lcd_write_reg(0xD312, 0x01);
    bsp_8080_lcd_write_reg(0xD313, 0x54);
    bsp_8080_lcd_write_reg(0xD314, 0x01);
    bsp_8080_lcd_write_reg(0xD315, 0x82);
    bsp_8080_lcd_write_reg(0xD316, 0x01);
    bsp_8080_lcd_write_reg(0xD317, 0xCA);
    bsp_8080_lcd_write_reg(0xD318, 0x02);
    bsp_8080_lcd_write_reg(0xD319, 0x00);
    bsp_8080_lcd_write_reg(0xD31A, 0x02);
    bsp_8080_lcd_write_reg(0xD31B, 0x01);
    bsp_8080_lcd_write_reg(0xD31C, 0x02);
    bsp_8080_lcd_write_reg(0xD31D, 0x34);
    bsp_8080_lcd_write_reg(0xD31E, 0x02);
    bsp_8080_lcd_write_reg(0xD31F, 0x67);
    bsp_8080_lcd_write_reg(0xD320, 0x02);
    bsp_8080_lcd_write_reg(0xD321, 0x84);
    bsp_8080_lcd_write_reg(0xD322, 0x02);
    bsp_8080_lcd_write_reg(0xD323, 0xA4);
    bsp_8080_lcd_write_reg(0xD324, 0x02);
    bsp_8080_lcd_write_reg(0xD325, 0xB7);
    bsp_8080_lcd_write_reg(0xD326, 0x02);
    bsp_8080_lcd_write_reg(0xD327, 0xCF);
    bsp_8080_lcd_write_reg(0xD328, 0x02);
    bsp_8080_lcd_write_reg(0xD329, 0xDE);
    bsp_8080_lcd_write_reg(0xD32A, 0x02);
    bsp_8080_lcd_write_reg(0xD32B, 0xF2);
    bsp_8080_lcd_write_reg(0xD32C, 0x02);
    bsp_8080_lcd_write_reg(0xD32D, 0xFE);
    bsp_8080_lcd_write_reg(0xD32E, 0x03);
    bsp_8080_lcd_write_reg(0xD32F, 0x10);
    bsp_8080_lcd_write_reg(0xD330, 0x03);
    bsp_8080_lcd_write_reg(0xD331, 0x33);
    bsp_8080_lcd_write_reg(0xD332, 0x03);
    bsp_8080_lcd_write_reg(0xD333, 0x6D);
    bsp_8080_lcd_write_reg(0xD400, 0x00);
    bsp_8080_lcd_write_reg(0xD401, 0x33);
    bsp_8080_lcd_write_reg(0xD402, 0x00);
    bsp_8080_lcd_write_reg(0xD403, 0x34);
    bsp_8080_lcd_write_reg(0xD404, 0x00);
    bsp_8080_lcd_write_reg(0xD405, 0x3A);
    bsp_8080_lcd_write_reg(0xD406, 0x00);
    bsp_8080_lcd_write_reg(0xD407, 0x4A);
    bsp_8080_lcd_write_reg(0xD408, 0x00);
    bsp_8080_lcd_write_reg(0xD409, 0x5C);
    bsp_8080_lcd_write_reg(0xD40A, 0x00);
    bsp_8080_lcd_write_reg(0xD40B, 0x81);

    bsp_8080_lcd_write_reg(0xD40C, 0x00);
    bsp_8080_lcd_write_reg(0xD40D, 0xA6);
    bsp_8080_lcd_write_reg(0xD40E, 0x00);
    bsp_8080_lcd_write_reg(0xD40F, 0xE5);
    bsp_8080_lcd_write_reg(0xD410, 0x01);
    bsp_8080_lcd_write_reg(0xD411, 0x13);
    bsp_8080_lcd_write_reg(0xD412, 0x01);
    bsp_8080_lcd_write_reg(0xD413, 0x54);
    bsp_8080_lcd_write_reg(0xD414, 0x01);
    bsp_8080_lcd_write_reg(0xD415, 0x82);
    bsp_8080_lcd_write_reg(0xD416, 0x01);
    bsp_8080_lcd_write_reg(0xD417, 0xCA);
    bsp_8080_lcd_write_reg(0xD418, 0x02);
    bsp_8080_lcd_write_reg(0xD419, 0x00);
    bsp_8080_lcd_write_reg(0xD41A, 0x02);
    bsp_8080_lcd_write_reg(0xD41B, 0x01);
    bsp_8080_lcd_write_reg(0xD41C, 0x02);
    bsp_8080_lcd_write_reg(0xD41D, 0x34);
    bsp_8080_lcd_write_reg(0xD41E, 0x02);
    bsp_8080_lcd_write_reg(0xD41F, 0x67);
    bsp_8080_lcd_write_reg(0xD420, 0x02);
    bsp_8080_lcd_write_reg(0xD421, 0x84);
    bsp_8080_lcd_write_reg(0xD422, 0x02);
    bsp_8080_lcd_write_reg(0xD423, 0xA4);
    bsp_8080_lcd_write_reg(0xD424, 0x02);
    bsp_8080_lcd_write_reg(0xD425, 0xB7);
    bsp_8080_lcd_write_reg(0xD426, 0x02);
    bsp_8080_lcd_write_reg(0xD427, 0xCF);
    bsp_8080_lcd_write_reg(0xD428, 0x02);
    bsp_8080_lcd_write_reg(0xD429, 0xDE);
    bsp_8080_lcd_write_reg(0xD42A, 0x02);
    bsp_8080_lcd_write_reg(0xD42B, 0xF2);
    bsp_8080_lcd_write_reg(0xD42C, 0x02);
    bsp_8080_lcd_write_reg(0xD42D, 0xFE);
    bsp_8080_lcd_write_reg(0xD42E, 0x03);
    bsp_8080_lcd_write_reg(0xD42F, 0x10);
    bsp_8080_lcd_write_reg(0xD430, 0x03);
    bsp_8080_lcd_write_reg(0xD431, 0x33);
    bsp_8080_lcd_write_reg(0xD432, 0x03);
    bsp_8080_lcd_write_reg(0xD433, 0x6D);
    bsp_8080_lcd_write_reg(0xD500, 0x00);
    bsp_8080_lcd_write_reg(0xD501, 0x33);
    bsp_8080_lcd_write_reg(0xD502, 0x00);
    bsp_8080_lcd_write_reg(0xD503, 0x34);
    bsp_8080_lcd_write_reg(0xD504, 0x00);
    bsp_8080_lcd_write_reg(0xD505, 0x3A);
    bsp_8080_lcd_write_reg(0xD506, 0x00);
    bsp_8080_lcd_write_reg(0xD507, 0x4A);
    bsp_8080_lcd_write_reg(0xD508, 0x00);
    bsp_8080_lcd_write_reg(0xD509, 0x5C);
    bsp_8080_lcd_write_reg(0xD50A, 0x00);
    bsp_8080_lcd_write_reg(0xD50B, 0x81);

    bsp_8080_lcd_write_reg(0xD50C, 0x00);
    bsp_8080_lcd_write_reg(0xD50D, 0xA6);
    bsp_8080_lcd_write_reg(0xD50E, 0x00);
    bsp_8080_lcd_write_reg(0xD50F, 0xE5);
    bsp_8080_lcd_write_reg(0xD510, 0x01);
    bsp_8080_lcd_write_reg(0xD511, 0x13);
    bsp_8080_lcd_write_reg(0xD512, 0x01);
    bsp_8080_lcd_write_reg(0xD513, 0x54);
    bsp_8080_lcd_write_reg(0xD514, 0x01);
    bsp_8080_lcd_write_reg(0xD515, 0x82);
    bsp_8080_lcd_write_reg(0xD516, 0x01);
    bsp_8080_lcd_write_reg(0xD517, 0xCA);
    bsp_8080_lcd_write_reg(0xD518, 0x02);
    bsp_8080_lcd_write_reg(0xD519, 0x00);
    bsp_8080_lcd_write_reg(0xD51A, 0x02);
    bsp_8080_lcd_write_reg(0xD51B, 0x01);
    bsp_8080_lcd_write_reg(0xD51C, 0x02);
    bsp_8080_lcd_write_reg(0xD51D, 0x34);
    bsp_8080_lcd_write_reg(0xD51E, 0x02);
    bsp_8080_lcd_write_reg(0xD51F, 0x67);
    bsp_8080_lcd_write_reg(0xD520, 0x02);
    bsp_8080_lcd_write_reg(0xD521, 0x84);
    bsp_8080_lcd_write_reg(0xD522, 0x02);
    bsp_8080_lcd_write_reg(0xD523, 0xA4);
    bsp_8080_lcd_write_reg(0xD524, 0x02);
    bsp_8080_lcd_write_reg(0xD525, 0xB7);
    bsp_8080_lcd_write_reg(0xD526, 0x02);
    bsp_8080_lcd_write_reg(0xD527, 0xCF);
    bsp_8080_lcd_write_reg(0xD528, 0x02);
    bsp_8080_lcd_write_reg(0xD529, 0xDE);
    bsp_8080_lcd_write_reg(0xD52A, 0x02);
    bsp_8080_lcd_write_reg(0xD52B, 0xF2);
    bsp_8080_lcd_write_reg(0xD52C, 0x02);
    bsp_8080_lcd_write_reg(0xD52D, 0xFE);
    bsp_8080_lcd_write_reg(0xD52E, 0x03);
    bsp_8080_lcd_write_reg(0xD52F, 0x10);
    bsp_8080_lcd_write_reg(0xD530, 0x03);
    bsp_8080_lcd_write_reg(0xD531, 0x33);
    bsp_8080_lcd_write_reg(0xD532, 0x03);
    bsp_8080_lcd_write_reg(0xD533, 0x6D);
    bsp_8080_lcd_write_reg(0xD600, 0x00);
    bsp_8080_lcd_write_reg(0xD601, 0x33);
    bsp_8080_lcd_write_reg(0xD602, 0x00);
    bsp_8080_lcd_write_reg(0xD603, 0x34);
    bsp_8080_lcd_write_reg(0xD604, 0x00);
    bsp_8080_lcd_write_reg(0xD605, 0x3A);
    bsp_8080_lcd_write_reg(0xD606, 0x00);
    bsp_8080_lcd_write_reg(0xD607, 0x4A);
    bsp_8080_lcd_write_reg(0xD608, 0x00);
    bsp_8080_lcd_write_reg(0xD609, 0x5C);
    bsp_8080_lcd_write_reg(0xD60A, 0x00);
    bsp_8080_lcd_write_reg(0xD60B, 0x81);

    bsp_8080_lcd_write_reg(0xD60C, 0x00);
    bsp_8080_lcd_write_reg(0xD60D, 0xA6);
    bsp_8080_lcd_write_reg(0xD60E, 0x00);
    bsp_8080_lcd_write_reg(0xD60F, 0xE5);
    bsp_8080_lcd_write_reg(0xD610, 0x01);
    bsp_8080_lcd_write_reg(0xD611, 0x13);
    bsp_8080_lcd_write_reg(0xD612, 0x01);
    bsp_8080_lcd_write_reg(0xD613, 0x54);
    bsp_8080_lcd_write_reg(0xD614, 0x01);
    bsp_8080_lcd_write_reg(0xD615, 0x82);
    bsp_8080_lcd_write_reg(0xD616, 0x01);
    bsp_8080_lcd_write_reg(0xD617, 0xCA);
    bsp_8080_lcd_write_reg(0xD618, 0x02);
    bsp_8080_lcd_write_reg(0xD619, 0x00);
    bsp_8080_lcd_write_reg(0xD61A, 0x02);
    bsp_8080_lcd_write_reg(0xD61B, 0x01);
    bsp_8080_lcd_write_reg(0xD61C, 0x02);
    bsp_8080_lcd_write_reg(0xD61D, 0x34);
    bsp_8080_lcd_write_reg(0xD61E, 0x02);
    bsp_8080_lcd_write_reg(0xD61F, 0x67);
    bsp_8080_lcd_write_reg(0xD620, 0x02);
    bsp_8080_lcd_write_reg(0xD621, 0x84);
    bsp_8080_lcd_write_reg(0xD622, 0x02);
    bsp_8080_lcd_write_reg(0xD623, 0xA4);
    bsp_8080_lcd_write_reg(0xD624, 0x02);
    bsp_8080_lcd_write_reg(0xD625, 0xB7);
    bsp_8080_lcd_write_reg(0xD626, 0x02);
    bsp_8080_lcd_write_reg(0xD627, 0xCF);
    bsp_8080_lcd_write_reg(0xD628, 0x02);
    bsp_8080_lcd_write_reg(0xD629, 0xDE);
    bsp_8080_lcd_write_reg(0xD62A, 0x02);
    bsp_8080_lcd_write_reg(0xD62B, 0xF2);
    bsp_8080_lcd_write_reg(0xD62C, 0x02);
    bsp_8080_lcd_write_reg(0xD62D, 0xFE);
    bsp_8080_lcd_write_reg(0xD62E, 0x03);
    bsp_8080_lcd_write_reg(0xD62F, 0x10);
    bsp_8080_lcd_write_reg(0xD630, 0x03);
    bsp_8080_lcd_write_reg(0xD631, 0x33);
    bsp_8080_lcd_write_reg(0xD632, 0x03);
    bsp_8080_lcd_write_reg(0xD633, 0x6D);
    /* LV2 Page 0 enable */
    bsp_8080_lcd_write_reg(0xF000, 0x55);
    bsp_8080_lcd_write_reg(0xF001, 0xAA);
    bsp_8080_lcd_write_reg(0xF002, 0x52);
    bsp_8080_lcd_write_reg(0xF003, 0x08);
    bsp_8080_lcd_write_reg(0xF004, 0x00);
    /* Display control */
    bsp_8080_lcd_write_reg(0xB100, 0xCC);
    bsp_8080_lcd_write_reg(0xB101, 0x00);
    /* Source hold time */
    bsp_8080_lcd_write_reg(0xB600, 0x05);
    /* Gate EQ control */
    bsp_8080_lcd_write_reg(0xB700, 0x70);
    bsp_8080_lcd_write_reg(0xB701, 0x70);
    /* Source EQ control (Mode 2) */
    bsp_8080_lcd_write_reg(0xB800, 0x01);
    bsp_8080_lcd_write_reg(0xB801, 0x03);
    bsp_8080_lcd_write_reg(0xB802, 0x03);
    bsp_8080_lcd_write_reg(0xB803, 0x03);
    /* Inversion mode (2-dot) */
    bsp_8080_lcd_write_reg(0xBC00, 0x02);
    bsp_8080_lcd_write_reg(0xBC01, 0x00);
    bsp_8080_lcd_write_reg(0xBC02, 0x00);
    /* Timing control 4H w/ 4-delay */
    bsp_8080_lcd_write_reg(0xC900, 0xD0);
    bsp_8080_lcd_write_reg(0xC901, 0x02);
    bsp_8080_lcd_write_reg(0xC902, 0x50);
    bsp_8080_lcd_write_reg(0xC903, 0x50);
    bsp_8080_lcd_write_reg(0xC904, 0x50);
    bsp_8080_lcd_write_reg(0x3500, 0x00);
    bsp_8080_lcd_write_reg(0x3A00, 0x55); /* 16-bit/pixel */
    bsp_8080_lcd_wr_regno(0x1100);
    delay_us(120);
    bsp_8080_lcd_wr_regno(0x2900);
}


















