// #include "MPU.h"

// /**
//  * @brief 获取 MPU 支持的硬件区域数量
//  * @return 区域数量（通常为 8 或 16）
//  */
// uint32_t MPU_GetRegionCount(void)
// {
//     return ((MPU->TYPE >> MPU_TYPE_IREGION_Pos) & MPU_TYPE_IREGION_Msk);
// }

// /**
//  * @brief 配置一个 MPU 区域
//  * @param config 区域配置结构体指针
//  */
// void MPU_ConfigRegion(MPU_RegionConfig_t *config)
// {
//     uint32_t rasr = 0;

//     /* 1. 禁用中断，避免配置过程被打断（可选） */
//     __disable_irq();

//     /* 2. 选择区域编号 */
//     MPU->RNR = config->region_num;

//     /* 3. 配置区域基址（必须对齐到大小） */
//     MPU->RBAR = (config->base_addr & MPU_RBAR_ADDR_Msk);

//     /* 4. 构建 RASR 寄存器值 */
//     /* 大小编码 */
//     rasr |= (config->size << MPU_RASR_SIZE_Pos);
//     /* 子区域禁用掩码 */
//     rasr |= ((config->subregion_disable & 0xFFU) << MPU_RASR_SRD_Pos);
//     /* 访问权限 */
//     rasr |= ((config->access_permission & 0x7U) << MPU_RASR_AP_Pos);
//     /* 可执行属性 (XN) */
//     if (config->attribute & MPU_ATTR_EXEC_NEVER) {
//         rasr |= MPU_RASR_XN_Msk;
//     }
//     /* 内存属性 (C, B, S) */
//     uint32_t attr = config->attribute & (~MPU_ATTR_EXEC_NEVER);
//     switch (attr) {
//         case MPU_ATTR_NORMAL_NOCACHE:
//             /* TEX=0, C=0, B=0, S=0 */
//             break;
//         case MPU_ATTR_NORMAL_WT:
//             /* TEX=0, C=1, B=0, S=0 */
//             rasr |= MPU_RASR_C_Msk;
//             break;
//         case MPU_ATTR_NORMAL_WB:
//             /* TEX=0, C=1, B=1, S=0 */
//             rasr |= (MPU_RASR_C_Msk | MPU_RASR_B_Msk);
//             break;
//         case MPU_ATTR_DEVICE:
//             /* TEX=0, C=0, B=1, S=1 */
//             rasr |= (MPU_RASR_B_Msk | MPU_RASR_S_Msk);
//             break;
//         case MPU_ATTR_STRONGLY_ORDERED:
//             /* TEX=0, C=0, B=0, S=1 */
//             rasr |= MPU_RASR_S_Msk;
//             break;
//         default:
//             /* 不支持的属性，保持默认（可缓存） */
//             break;
//     }
//     /* 使能区域 */
//     rasr |= MPU_RASR_ENABLE_Msk;

//     MPU->RASR = rasr;

//     /* 5. 恢复中断 */
//     __enable_irq();
// }

// /**
//  * @brief 初始化 MPU：禁用所有区域，并配置默认区域（可选）
//  */
// void MPU_Init(void)
// {
//     uint32_t i;
//     uint32_t region_count = MPU_GetRegionCount();

//     /* 禁用 MPU */
//     MPU_Disable();

//     /* 将所有区域配置为无效（禁用） */
//     for (i = 0; i < region_count; i++) {
//         MPU->RNR = i;
//         MPU->RASR = 0;   /* 清除使能位及其他配置 */
//     }

//     /* 可选：配置一个默认区域（例如整个内存空间），但通常不需要 */
// }

// /**
//  * @brief 使能 MPU
//  * @param enable_priv_default 是否使能特权默认内存映射 (0/1)
//  * @param enable_hfault      是否在硬故障时启用 MPU (0/1)
//  */
// void MPU_Enable(uint8_t enable_priv_default, uint8_t enable_hfault)
// {
//     uint32_t ctrl = MPU_CTRL_ENABLE_Msk;

//     if (enable_priv_default) {
//         ctrl |= MPU_CTRL_PRIVDEFENA_Msk;
//     }
//     if (enable_hfault) {
//         ctrl |= MPU_CTRL_HFNMIENA_Msk;
//     }

//     MPU->CTRL = ctrl;

//     /* 数据同步屏障，确保配置生效 */
//     __DSB();
//     __ISB();
// }

// /**
//  * @brief 禁用 MPU
//  */
// void MPU_Disable(void)
// {
//     MPU->CTRL &= ~MPU_CTRL_ENABLE_Msk;
//     __DSB();
//     __ISB();
// }








/**
 ****************************************************************************************************
 * @file        mpu.c
 * @version     V1.0
 * @brief       MPU内存保护 驱动代码
 ****************************************************************************************************
 * @attention   Waiken-Smart 慧勤智远
 *
 * 实验平台:    GD32H757ZMT6小系统板
 *
 ****************************************************************************************************
 */
 
#include "mpu.h"


/**
 * @brief       设置某个区域的MPU保护
 * @param       baseaddr:MPU保护区域的基地址(首地址)
 * @param       size:MPU保护区域的大小(单位为字节，必须大于等于32字节)
 * @param       rnum:MPU保护区域编号,范围:0~15,最大支持15个保护区域
 * @param       de:禁止指令访问;MPU_INSTRUCTION_EXEC_PERMIT,允许指令访问;MPU_INSTRUCTION_EXEC_NOT_PERMIT,禁止指令访问
 * @param       ap:访问权限,访问关系如下:
 *   @arg       MPU_AP_NO_ACCESS,无访问（特权&用户都不可访问）
 *   @arg       MPU_AP_PRIV_RW,仅支持特权读写访问
 *   @arg       MPU_AP_PRIV_RW_UNPRIV_RO,禁止用户写访问（特权可读写访问）
 *   @arg       MPU_AP_FULL_ACCESS,全访问（特权&用户都可以访问）
 *   @arg       MPU_AP_PRIV_RO,仅支持特权读访问
 *   @arg       MPU_AP_PRIV_UNPRIV_RO,只读（特权&用户都不可以写）
 * @param       sen : 是否允许共用;MPU_ACCESS_NON_SHAREABLE,不允许;MPU_ACCESS_SHAREABLE,允许
 * @param       cen : 是否允许cache;MPU_ACCESS_NON_CACHEABLE,不允许;MPU_ACCESS_CACHEABLE,允许
 * @param       ben : 是否允许缓冲;MPU_ACCESS_NON_BUFFERABLE,不允许;MPU_ACCESS_BUFFERABLE,允许
 * @retval      无
 */
void mpu_set_protection(uint32_t baseaddr, uint32_t size, uint32_t rnum, uint8_t de, uint8_t ap, uint8_t sen, uint8_t cen, uint8_t ben)
{
    mpu_region_init_struct mpu_init_struct;
    mpu_region_struct_para_init(&mpu_init_struct);

    ARM_MPU_Disable();                                           /* 配置MPU之前先关闭MPU,配置完成以后再使能MPU */
  
    mpu_init_struct.region_number       = rnum;                  /* 设置保护区域编号 */
    mpu_init_struct.region_base_address = baseaddr;              /* 设置保护区域基地址 */
    mpu_init_struct.instruction_exec    = de;                    /* 是否允许指令访问 */
    mpu_init_struct.access_permission   = ap;                    /* 设置访问权限 */
    mpu_init_struct.tex_type            = MPU_TEX_TYPE0;         /* 设置TEX类型为type 0 */
    mpu_init_struct.access_shareable    = sen;                   /* 是否共用? */
    mpu_init_struct.access_cacheable    = cen;                   /* 是否cache? */
    mpu_init_struct.access_bufferable   = ben;                   /* 是否缓冲? */
    mpu_init_struct.subregion_disable   = 0X00;                  /* 禁止子区域 */
    mpu_init_struct.region_size         = size;                  /* 设置保护区域大小 */    
    mpu_region_config(&mpu_init_struct);                         /* 配置MPU区域 */
    mpu_region_enable();                                         /* 使能MPU区域 */
                
    ARM_MPU_Enable(MPU_MODE_PRIV_DEFAULT);                       /* 设置完毕,使能MPU保护 */
}
 
/**
 * @brief       设置需要保护的存储块
 * @note        必须对部分存储区域进行MPU保护,否则可能导致程序运行异常
 *              比如MCU屏不显示,摄像头采集数据出错等等问题...
 * @param       无
 * @retval      无
 */
void mpu_memory_protection(void)
{
    /* 保护整个ITCM,共64K字节 */
    mpu_set_protection( 0x00000000,                 /* 区域基地址 */
                        MPU_REGION_SIZE_64KB,       /* 区域大小 */
                        MPU_REGION_NUMBER0,         /* 区域编号 */
                        MPU_INSTRUCTION_EXEC_PERMIT,/* 允许指令访问 */
                        MPU_AP_FULL_ACCESS,         /* 全访问 */
                        MPU_ACCESS_NON_SHAREABLE,   /* 禁止共用 */
                        MPU_ACCESS_CACHEABLE,       /* 允许cache */
                        MPU_ACCESS_BUFFERABLE);     /* 允许缓冲 */

    /* 保护整个DTCM,共128K字节 */
    mpu_set_protection( 0x20000000,                 /* 区域基地址 */
                        MPU_REGION_SIZE_128KB,      /* 区域大小 */
                        MPU_REGION_NUMBER1,         /* 区域编号 */
                        MPU_INSTRUCTION_EXEC_PERMIT,/* 允许指令访问 */
                        MPU_AP_FULL_ACCESS,         /* 全访问 */
                        MPU_ACCESS_NON_SHAREABLE,   /* 禁止共用 */
                        MPU_ACCESS_CACHEABLE,       /* 允许cache */
                        MPU_ACCESS_BUFFERABLE);     /* 允许缓冲 */

    /* 保护整个AXI SRAM,共1024K字节 */
    mpu_set_protection( 0x24000000,                 /* 区域基地址 */
                        MPU_REGION_SIZE_1MB,        /* 区域大小 */
                        MPU_REGION_NUMBER2,         /* 区域编号 */
                        MPU_INSTRUCTION_EXEC_PERMIT,/* 允许指令访问 */
                        MPU_AP_FULL_ACCESS,         /* 全访问 */
                        MPU_ACCESS_SHAREABLE,       /* 允许共用 */
                        MPU_ACCESS_CACHEABLE,       /* 允许cache */
                        MPU_ACCESS_NON_BUFFERABLE); /* 禁止缓冲 */

    /* 保护整个SRAM0和SRAM1,共32K字节 */
    mpu_set_protection( 0x30000000,                 /* 区域基地址 */
                        MPU_REGION_SIZE_32KB,       /* 区域大小 */
                        MPU_REGION_NUMBER3,         /* 区域编号 */
                        MPU_INSTRUCTION_EXEC_PERMIT,/* 允许指令访问 */
                        MPU_AP_FULL_ACCESS,         /* 全访问 */
                        MPU_ACCESS_NON_SHAREABLE,   /* 禁止共用 */
                        MPU_ACCESS_CACHEABLE,       /* 允许cache */
                        MPU_ACCESS_BUFFERABLE);     /* 允许缓冲 */

    /* 保护MCU LCD屏所在的EXMC区域,共64M字节 */
    mpu_set_protection( 0x64000000,                 /* 区域基地址 */
                        MPU_REGION_SIZE_64MB,       /* 区域大小 */
                        MPU_REGION_NUMBER4,         /* 区域编号 */
                        MPU_INSTRUCTION_EXEC_PERMIT,/* 允许指令访问 */
                        MPU_AP_FULL_ACCESS,         /* 全访问 */
                        MPU_ACCESS_NON_SHAREABLE,   /* 禁止共用 */
                        MPU_ACCESS_NON_CACHEABLE,   /* 禁止cache */
                        MPU_ACCESS_NON_BUFFERABLE); /* 禁止缓冲 */
                        
    /* 保护SDRAM区域,共32M字节 */
    mpu_set_protection( 0xC0000000,                 /* 区域基地址 */
                        MPU_REGION_SIZE_32MB,       /* 区域大小 */
                        MPU_REGION_NUMBER5,         /* 区域编号 */
                        MPU_INSTRUCTION_EXEC_PERMIT,/* 允许指令访问 */
                        MPU_AP_FULL_ACCESS,         /* 全访问 */
                        MPU_ACCESS_NON_SHAREABLE,   /* 禁止共用 */
                        MPU_ACCESS_CACHEABLE,       /* 允许cache */
                        MPU_ACCESS_BUFFERABLE);     /* 允许缓冲 */                               
}
















