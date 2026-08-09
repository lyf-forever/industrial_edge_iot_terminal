/*!
 * 文件名称： driver_exmc.c
 * 描    述： exmc 驱动文件
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

#include "driver_exmc.h"


/*!
* 说明     emxc gpio 初始化
* 输入[1]  无
* 返回值   无
*/
static void driver_exmc_nor_sram_gpio_init(void)
{
   /* EXMC clock enable */
    rcu_periph_clock_enable(RCU_EXMC);

    /* EXMC enable */
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_GPIOF);
    rcu_periph_clock_enable(RCU_GPIOG);

    /* configure GPIO D[0-15] */
    gpio_af_set(GPIOD, GPIO_AF_12, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 |
                GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 |
                  GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 |
                            GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);

    gpio_af_set(GPIOE, GPIO_AF_12, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
                GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
                  GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
                            GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
}

static void driver_exmc_sdram_gpio_init(void) 
{
   /* EXMC clock enable */
    rcu_periph_clock_enable(RCU_EXMC);    
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_GPIOF);
    rcu_periph_clock_enable(RCU_GPIOG);
    
    /* D12(PC0) pin configuration */
    gpio_af_set(GPIOA, GPIO_AF_12, GPIO_PIN_7);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_7);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_7);    

    /* common GPIO configuration */
    /* SDNE0(PC2),SDCKE0(PC3) pin configuration */
    gpio_af_set(GPIOD, GPIO_AF_12, BITS(0,1)|BITS(8,10)|BITS(14,15));
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP,BITS(0,1)|BITS(8,10)|BITS(14,15));
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, BITS(0,1)|BITS(8,10)|BITS(14,15));
    
    /* common GPIO configuration */
    /* SDNE0(PC2),SDCKE0(PC3) pin configuration */
    gpio_af_set(GPIOC, GPIO_AF_12, GPIO_PIN_4|GPIO_PIN_5);
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP,GPIO_PIN_4|GPIO_PIN_5);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_4|GPIO_PIN_5);  

    /* D12(PC0) pin configuration */
    gpio_af_set(GPIOE, GPIO_AF_12, GPIO_PIN_0|GPIO_PIN_1|BITS(7,15));
    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0|GPIO_PIN_1|BITS(7,15));
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_0|GPIO_PIN_1|BITS(7,15));
    
    /* D12(PC0) pin configuration */
    gpio_af_set(GPIOF, GPIO_AF_12, BITS(0,5)|BITS(11,15));
    gpio_mode_set(GPIOF, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BITS(0,5)|BITS(11,15));
    gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, BITS(0,5)|BITS(11,15));
    
    /* D12(PC0) pin configuration */
    gpio_af_set(GPIOG, GPIO_AF_12, BITS(0,2)|BITS(4,5)|GPIO_PIN_8|GPIO_PIN_15);
    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BITS(0,2)|BITS(4,5)|GPIO_PIN_8|GPIO_PIN_15);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, BITS(0,2)|BITS(4,5)|GPIO_PIN_8|GPIO_PIN_15); 

}


/*!
* 说明     emxc LCD模式通用gpio初始化
* 输入[1]  norsram_region: @EXMC_BANK0_NORSRAM_REGION0/EXMC_BANK0_NORSRAM_REGION1/EXMC_BANK0_NORSRAM_REGION2/EXMC_BANK0_NORSRAM_REGION3
* 返回值   无
*/
static void driver_exmc_lcd_16bit_gpio_init(void)
{
   /* EXMC clock enable */
    rcu_periph_clock_enable(RCU_EXMC);

    /* EXMC enable */
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_GPIOF);
    rcu_periph_clock_enable(RCU_GPIOG);

    /* configure GPIO D[0-15] */
    gpio_af_set(GPIOD, GPIO_AF_12, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 |
                GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 |
                  GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 |
                            GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);

    gpio_af_set(GPIOE, GPIO_AF_12, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
                GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
                  GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
                            GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
                            
    /* configure NOE NWE */
    gpio_af_set(GPIOD, GPIO_AF_12, GPIO_PIN_4 | GPIO_PIN_5);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_4 | GPIO_PIN_5);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_4 | GPIO_PIN_5);                            
}



/*!
* 说明     emxc nor/sram模式初始化
* 输入[1]  norsram_region: @EXMC_BANK0_NORSRAM_REGION0/EXMC_BANK0_NORSRAM_REGION1/EXMC_BANK0_NORSRAM_REGION2/EXMC_BANK0_NORSRAM_REGION3
* 返回值   无
*/
void driver_exmc_norsram_init(uint32_t norsram_region)
{    
    #define NOR_SRAM_READ_DATA_SETTIME_NS 30
    #define NOR_SRAM_READ_ADDR_SETTIME_NS 10

    #define NOR_SRAM_WRITE_DATA_SETTIME_NS 30
    #define NOR_SRAM_WRITE_ADDR_SETTIME_NS 10
    
    exmc_norsram_parameter_struct nor_init_struct;
    exmc_norsram_timing_parameter_struct nor_timing_read_init_struct;
    exmc_norsram_timing_parameter_struct nor_timing_write_init_struct;

    /* EXMC clock enable */
    rcu_periph_clock_enable(RCU_EXMC);

    driver_exmc_nor_sram_gpio_init();
        
    nor_init_struct.read_write_timing = &nor_timing_read_init_struct;       
    nor_init_struct.write_timing = &nor_timing_write_init_struct;
    
    exmc_norsram_struct_para_init(&nor_init_struct);
    
    SystemCoreClockUpdate();
        
    /* config timing parameter */
    nor_timing_read_init_struct.asyn_access_mode = EXMC_ACCESS_MODE_A;
    nor_timing_read_init_struct.syn_data_latency = EXMC_DATALAT_2_CLK;
    nor_timing_read_init_struct.syn_clk_division = EXMC_SYN_CLOCK_RATIO_2_CLK;
    nor_timing_read_init_struct.bus_latency = 1;
    nor_timing_read_init_struct.asyn_data_setuptime = NOR_SRAM_READ_DATA_SETTIME_NS/(float)(1000000000.0f/SystemCoreClock);
    nor_timing_read_init_struct.asyn_address_holdtime = 1;
    nor_timing_read_init_struct.asyn_address_setuptime = NOR_SRAM_READ_ADDR_SETTIME_NS/(float)(1000000000.0f/SystemCoreClock);
    
    /* config timing parameter */
    nor_timing_write_init_struct.asyn_access_mode = EXMC_ACCESS_MODE_A;
    nor_timing_write_init_struct.syn_data_latency = EXMC_DATALAT_2_CLK;
    nor_timing_write_init_struct.syn_clk_division = EXMC_SYN_CLOCK_RATIO_2_CLK;
    nor_timing_write_init_struct.bus_latency = 1;
    nor_timing_write_init_struct.asyn_data_setuptime = NOR_SRAM_WRITE_DATA_SETTIME_NS/(float)(1000000000.0f/SystemCoreClock);
    nor_timing_write_init_struct.asyn_address_holdtime = 1;
    nor_timing_write_init_struct.asyn_address_setuptime =NOR_SRAM_WRITE_ADDR_SETTIME_NS/(float)(1000000000.0f/SystemCoreClock); 

    /* config EXMC bus parameters */
    nor_init_struct.norsram_region = norsram_region;
    nor_init_struct.write_mode = EXMC_ASYN_WRITE;
    nor_init_struct.extended_mode = ENABLE;
    nor_init_struct.asyn_wait = DISABLE;
    nor_init_struct.nwait_signal = DISABLE;
    nor_init_struct.memory_write = ENABLE;
    nor_init_struct.nwait_config = EXMC_NWAIT_CONFIG_BEFORE;
//    nor_init_struct.wrap_burst_mode = DISABLE;
    nor_init_struct.nwait_polarity = EXMC_NWAIT_POLARITY_LOW;
    nor_init_struct.burst_mode = DISABLE;
    nor_init_struct.databus_width = EXMC_NOR_DATABUS_WIDTH_16B;
    nor_init_struct.memory_type = EXMC_MEMORY_TYPE_SRAM;
    nor_init_struct.address_data_mux = DISABLE;

    exmc_norsram_init(&nor_init_struct);

    /* enable the EXMC bank0 NORSRAM */
    exmc_norsram_enable(norsram_region);   
}


/*!
* 说明     emxc LCD模式初始化
* 输入[1]  norsram_region: @EXMC_BANK0_NORSRAM_REGION0/EXMC_BANK0_NORSRAM_REGION1/EXMC_BANK0_NORSRAM_REGION2/EXMC_BANK0_NORSRAM_REGION3
* 返回值   无
*/
void driver_exmc_lcd_init(uint32_t norsram_region)
{
    
    #define LCD_READ_DATA_SETTIME_NS 50
    #define LCD_READ_ADDR_SETTIME_NS 10

    #define LCD_WRITE_DATA_SETTIME_NS 20
    #define LCD_WRITE_ADDR_SETTIME_NS 10
    
    uint32_t ahb_clk=0;
    exmc_norsram_parameter_struct nor_init_struct;
    exmc_norsram_timing_parameter_struct nor_timing_read_init_struct;
    exmc_norsram_timing_parameter_struct nor_timing_write_init_struct;
    
    mpu_region_init_struct mpu_init_struct;
    mpu_region_struct_para_init(&mpu_init_struct);
    
    mpu_config(EXMC_BANK0_NORSRAM_REGIONx_ADDR(norsram_region),MPU_REGION_SIZE_64MB,MPU_AP_FULL_ACCESS,MPU_ACCESS_BUFFERABLE,MPU_ACCESS_NON_CACHEABLE,MPU_ACCESS_NON_SHAREABLE,MPU_INSTRUCTION_EXEC_NOT_PERMIT,MPU_TEX_TYPE0);    

    /* EXMC clock enable */
    rcu_periph_clock_enable(RCU_EXMC);
    
    driver_exmc_lcd_16bit_gpio_init();    
    
    nor_init_struct.read_write_timing = &nor_timing_read_init_struct;       
    nor_init_struct.write_timing = &nor_timing_write_init_struct;        
    exmc_norsram_struct_para_init(&nor_init_struct);    
    nor_init_struct.read_write_timing = &nor_timing_read_init_struct;       
    nor_init_struct.write_timing = &nor_timing_write_init_struct;      
    
    SystemCoreClockUpdate();
    ahb_clk=rcu_clock_freq_get(CK_AHB);
    /* config timing parameter */
    nor_timing_read_init_struct.asyn_access_mode = EXMC_ACCESS_MODE_A;
    nor_timing_read_init_struct.syn_data_latency = EXMC_DATALAT_2_CLK;
    nor_timing_read_init_struct.syn_clk_division = EXMC_SYN_CLOCK_RATIO_2_CLK;
    nor_timing_read_init_struct.bus_latency = 1;
    nor_timing_read_init_struct.asyn_data_setuptime = LCD_READ_DATA_SETTIME_NS/(float)(1000000000.0f/ahb_clk);
    nor_timing_read_init_struct.asyn_address_holdtime = 1;
    nor_timing_read_init_struct.asyn_address_setuptime = LCD_READ_ADDR_SETTIME_NS/(float)(1000000000.0f/ahb_clk);
    
    /* config timing parameter */
    nor_timing_write_init_struct.asyn_access_mode = EXMC_ACCESS_MODE_A;
    nor_timing_write_init_struct.syn_data_latency = EXMC_DATALAT_2_CLK;
    nor_timing_write_init_struct.syn_clk_division = EXMC_SYN_CLOCK_RATIO_2_CLK;
    nor_timing_write_init_struct.bus_latency = 1;
    nor_timing_write_init_struct.asyn_data_setuptime = LCD_WRITE_DATA_SETTIME_NS/(float)(1000000000.0f/ahb_clk);
    nor_timing_write_init_struct.asyn_address_holdtime = 1;
    nor_timing_write_init_struct.asyn_address_setuptime =LCD_WRITE_ADDR_SETTIME_NS/(float)(1000000000.0f/ahb_clk);

    /* config EXMC bus parameters */
    nor_init_struct.norsram_region = norsram_region;
    nor_init_struct.write_mode = EXMC_ASYN_WRITE;
    nor_init_struct.extended_mode = ENABLE;
    nor_init_struct.asyn_wait = DISABLE;
    nor_init_struct.nwait_signal = DISABLE;
    nor_init_struct.memory_write = ENABLE;
    nor_init_struct.nwait_config = EXMC_NWAIT_CONFIG_BEFORE;
//    nor_init_struct.wrap_burst_mode = DISABLE;
    nor_init_struct.nwait_polarity = EXMC_NWAIT_POLARITY_LOW;
    nor_init_struct.burst_mode = DISABLE;
    nor_init_struct.databus_width = EXMC_NOR_DATABUS_WIDTH_16B;
    nor_init_struct.memory_type = EXMC_MEMORY_TYPE_SRAM;
    nor_init_struct.address_data_mux = DISABLE;

    exmc_norsram_init(&nor_init_struct);

    /* enable the EXMC bank0 NORSRAM */
    exmc_norsram_enable(norsram_region);  
}

/*!
    \brief      software delay
    \param[in]  count: count value
    \param[out] none
    \retval     none
*/
static void _delay(uint32_t count)
{
    __IO uint32_t index = 0;

    for(index = (100 * count); index != 0; index--) {
    }
}

/*!
    \brief      sdram peripheral initialize
    \param[in]  sdram_device: specified SDRAM device
                EXMC_SDRAM_DEVICE0      
                EXMC_SDRAM_DEVICE1    
    \param[out] none
    \retval     none
*/
Drv_Err driver_exmc_sdram_init(uint32_t sdram_device)
{
    
    /* define mode register content */
    /* burst length */
    #define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000U)
    #define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001U)
    #define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002U)
    #define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0003U)

    /* burst type */
    #define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000U)
    #define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008U)

    /* CAS latency */
    #define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020U)
    #define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030U)

    /* write mode */
    #define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000U)
    #define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200U)

    #define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000U)

    #define SDRAM_TIMEOUT                            ((uint32_t)0x0000FFFFU)

    exmc_sdram_parameter_struct sdram_init_struct;
    exmc_sdram_timing_parameter_struct sdram_timing_init_struct;
    exmc_sdram_command_parameter_struct sdram_command_init_struct;
    exmc_sdram_struct_para_init(&sdram_init_struct);

    uint32_t command_content = 0, bank_select;
    uint32_t timeout = SDRAM_TIMEOUT;
        
   /* EXMC clock enable */
    rcu_periph_clock_enable(RCU_EXMC);
    
    driver_exmc_sdram_gpio_init();
 
    /* specify which SDRAM to read and write */
    if(EXMC_SDRAM_DEVICE0 == sdram_device) {
        bank_select = EXMC_SDRAM_DEVICE0_SELECT;
    } else {
        bank_select = EXMC_SDRAM_DEVICE1_SELECT;
    }

    /* EXMC SDRAM device initialization sequence --------------------------------*/
    /* step 1 : configure SDRAM timing registers --------------------------------*/
    /* LMRD: 2 clock cycles */
    sdram_timing_init_struct.load_mode_register_delay = 2;
    /* XSRD: min = 67ns */
    sdram_timing_init_struct.exit_selfrefresh_delay = 12;
    /* RASD: min=42ns , max=120k (ns) */
    sdram_timing_init_struct.row_address_select_delay = 8;
    /* ARFD: min=60ns */
    sdram_timing_init_struct.auto_refresh_delay = 11;
    /* WRD:  min=1 Clock cycles +6ns */
    sdram_timing_init_struct.write_recovery_delay = 2;
    /* RPD:  min=18ns */
    sdram_timing_init_struct.row_precharge_delay = 4;
    /* RCD:  min=18ns */
    sdram_timing_init_struct.row_to_column_delay = 4;

    /* step 2 : configure SDRAM control registers ---------------------------------*/
    sdram_init_struct.sdram_device = sdram_device;
    sdram_init_struct.column_address_width = EXMC_SDRAM_COW_ADDRESS_9;
    sdram_init_struct.row_address_width = EXMC_SDRAM_ROW_ADDRESS_13;
    sdram_init_struct.data_width = EXMC_SDRAM_DATABUS_WIDTH_16B;
    sdram_init_struct.internal_bank_number = EXMC_SDRAM_4_INTER_BANK;
    sdram_init_struct.cas_latency = EXMC_SDCLK_PERIODS_3_CK_EXMC;
    sdram_init_struct.write_protection = DISABLE;
    sdram_init_struct.sdclock_config = EXMC_SDCLK_PERIODS_2_CK_EXMC;
    sdram_init_struct.burst_read_switch = ENABLE;//DISABLE;//ENABLE;
    sdram_init_struct.pipeline_read_delay = EXMC_PIPELINE_DELAY_1_CK_EXMC;
    sdram_init_struct.timing = &sdram_timing_init_struct;
    /* EXMC SDRAM bank initialization */
    exmc_sdram_init(&sdram_init_struct);

    /* step 3 : configure CKE high command---------------------------------------*/
    sdram_command_init_struct.command = EXMC_SDRAM_CLOCK_ENABLE;
    sdram_command_init_struct.bank_select = bank_select;
    sdram_command_init_struct.auto_refresh_number = EXMC_SDRAM_AUTO_REFLESH_1_SDCLK;
    sdram_command_init_struct.mode_register_content = 0;
    /* wait until the SDRAM controller is ready */
    while((exmc_flag_get(sdram_device, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    
    if(timeout==0)
    {
        return DRV_ERROR;
    }
    /* send the command */
    exmc_sdram_command_config(&sdram_command_init_struct);

    /* step 4 : insert 10ms delay----------------------------------------------*/
    _delay(100);

    /* step 5 : configure precharge all command----------------------------------*/
    sdram_command_init_struct.command = EXMC_SDRAM_PRECHARGE_ALL;
    sdram_command_init_struct.bank_select = bank_select;
    sdram_command_init_struct.auto_refresh_number = EXMC_SDRAM_AUTO_REFLESH_1_SDCLK;
    sdram_command_init_struct.mode_register_content = 0;
    /* wait until the SDRAM controller is ready */
    timeout = SDRAM_TIMEOUT;
    while((exmc_flag_get(sdram_device, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    if(timeout==0)
    {
        return DRV_ERROR;
    }    
    /* send the command */
    exmc_sdram_command_config(&sdram_command_init_struct);

    /* step 6 : configure Auto-Refresh command-----------------------------------*/
    sdram_command_init_struct.command = EXMC_SDRAM_AUTO_REFRESH;
    sdram_command_init_struct.bank_select = bank_select;
    sdram_command_init_struct.auto_refresh_number = EXMC_SDRAM_AUTO_REFLESH_8_SDCLK;
    sdram_command_init_struct.mode_register_content = 0;
    /* wait until the SDRAM controller is ready */
    timeout = SDRAM_TIMEOUT;
    while((exmc_flag_get(sdram_device, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    if(timeout==0)
    {
        return DRV_ERROR;
    }    
    /* send the command */
    exmc_sdram_command_config(&sdram_command_init_struct);

    /* step 7 : configure load mode register command-----------------------------*/
    /* program mode register */
    command_content = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1        |
                      SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
                      SDRAM_MODEREG_CAS_LATENCY_3           |
                      SDRAM_MODEREG_OPERATING_MODE_STANDARD |
                      SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

    sdram_command_init_struct.command = EXMC_SDRAM_LOAD_MODE_REGISTER;
    sdram_command_init_struct.bank_select = bank_select;
    sdram_command_init_struct.auto_refresh_number = EXMC_SDRAM_AUTO_REFLESH_1_SDCLK;
    sdram_command_init_struct.mode_register_content = command_content;

    /* wait until the SDRAM controller is ready */
    timeout = SDRAM_TIMEOUT;
    while((exmc_flag_get(sdram_device, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    if(timeout==0)
    {
        return DRV_ERROR;
    }    
    /* send the command */
    exmc_sdram_command_config(&sdram_command_init_struct);

    /* step 8 : set the auto-refresh rate counter--------------------------------*/
    /* 64ms, 8192-cycle refresh, 64ms/8192=7.81us */
    /* SDCLK_Freq = SYS_Freq/2 */
    /* (7.81 us * SDCLK_Freq) - 20 */
    exmc_sdram_refresh_count_set(1542);

    /* wait until the SDRAM controller is ready */
    timeout = SDRAM_TIMEOUT;
    while((exmc_flag_get(sdram_device, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    if(timeout==0)
    {
        return DRV_ERROR;
    } 
    
    _delay(100);

    return DRV_SUCCESS;    
}
