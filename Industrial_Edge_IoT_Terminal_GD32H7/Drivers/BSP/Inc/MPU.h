// #ifndef __MPU_H
// #define __MPU_H

// #include "gd32h7xx.h"

// /* MPU 区域大小宏（单位：字节） */
// #define MPU_REGION_SIZE_32B       (0x04U)   /* 32 字节 */
// #define MPU_REGION_SIZE_64B       (0x05U)   /* 64 字节 */
// #define MPU_REGION_SIZE_128B      (0x06U)   /* 128 字节 */
// #define MPU_REGION_SIZE_256B      (0x07U)   /* 256 字节 */
// #define MPU_REGION_SIZE_512B      (0x08U)   /* 512 字节 */
// #define MPU_REGION_SIZE_1KB       (0x09U)   /* 1 KB */
// #define MPU_REGION_SIZE_2KB       (0x0AU)   /* 2 KB */
// #define MPU_REGION_SIZE_4KB       (0x0BU)   /* 4 KB */
// #define MPU_REGION_SIZE_8KB       (0x0CU)   /* 8 KB */
// #define MPU_REGION_SIZE_16KB      (0x0DU)   /* 16 KB */
// #define MPU_REGION_SIZE_32KB      (0x0EU)   /* 32 KB */
// #define MPU_REGION_SIZE_64KB      (0x0FU)   /* 64 KB */
// #define MPU_REGION_SIZE_128KB     (0x10U)   /* 128 KB */
// #define MPU_REGION_SIZE_256KB     (0x11U)   /* 256 KB */
// #define MPU_REGION_SIZE_512KB     (0x12U)   /* 512 KB */
// #define MPU_REGION_SIZE_1MB       (0x13U)   /* 1 MB */
// #define MPU_REGION_SIZE_2MB       (0x14U)   /* 2 MB */
// #define MPU_REGION_SIZE_4MB       (0x15U)   /* 4 MB */
// #define MPU_REGION_SIZE_8MB       (0x16U)   /* 8 MB */
// #define MPU_REGION_SIZE_16MB      (0x17U)   /* 16 MB */
// #define MPU_REGION_SIZE_32MB      (0x18U)   /* 32 MB */
// #define MPU_REGION_SIZE_64MB      (0x19U)   /* 64 MB */
// #define MPU_REGION_SIZE_128MB     (0x1AU)   /* 128 MB */
// #define MPU_REGION_SIZE_256MB     (0x1BU)   /* 256 MB */
// #define MPU_REGION_SIZE_512MB     (0x1CU)   /* 512 MB */
// #define MPU_REGION_SIZE_1GB       (0x1DU)   /* 1 GB */
// #define MPU_REGION_SIZE_2GB       (0x1EU)   /* 2 GB */
// #define MPU_REGION_SIZE_4GB       (0x1FU)   /* 4 GB */

// /* 访问权限 (AP) 宏 */
// #define MPU_AP_NO_ACCESS          (0x0U)   /* 无访问 */
// #define MPU_AP_PRIV_RW            (0x1U)   /* 特权可读写，用户不可访问 */
// #define MPU_AP_PRIV_RW_USER_RO    (0x2U)   /* 特权可读写，用户只读 */
// #define MPU_AP_PRIV_RW_USER_RW    (0x3U)   /* 特权+用户均可读写 */
// #define MPU_AP_PRIV_RO            (0x5U)   /* 特权只读，用户不可访问 */
// #define MPU_AP_PRIV_RO_USER_RO    (0x6U)   /* 特权+用户只读 */

// /* 属性 (XN, C, B, S) 组合宏 */
// #define MPU_ATTR_NORMAL_NOCACHE   (0x0U)   /* 非缓存，非共享 */
// #define MPU_ATTR_NORMAL_WT        (0x1U)   /* 正常内存，写通 */
// #define MPU_ATTR_NORMAL_WB        (0x2U)   /* 正常内存，写回 */
// #define MPU_ATTR_DEVICE           (0x4U)   /* 设备内存 */
// #define MPU_ATTR_STRONGLY_ORDERED (0x8U)   /* 强序内存 */
// #define MPU_ATTR_EXEC_NEVER       (0x10U)  /* 禁止执行 */

// /* 子区域禁用掩码（每1位对应一个子区域，LSB为子区域0）*/
// #define MPU_SRD_NONE              (0x00U)   /* 不禁用任何子区域 */
// #define MPU_SRD_ALL               (0xFFU)   /* 禁用所有子区域 */

// /**
//  * @brief MPU 区域配置结构体
//  */
// typedef struct {
//     uint32_t region_num;        /* 区域编号 (0-15) */
//     uint32_t base_addr;         /* 起始地址，必须对齐到大小 */
//     uint32_t size;              /* 区域大小，使用 MPU_REGION_SIZE_xxx 宏 */
//     uint32_t subregion_disable; /* 子区域禁用掩码 (0-255) */
//     uint32_t access_permission; /* 访问权限，使用 MPU_AP_xxx 宏 */
//     uint32_t attribute;         /* 属性，使用 MPU_ATTR_xxx 组合 */
// } MPU_RegionConfig_t;

// /* 函数声明 */
// void MPU_Init(void);
// void MPU_ConfigRegion(MPU_RegionConfig_t *config);
// void MPU_Enable(uint8_t enable_priv_default, uint8_t enable_hfault);
// void MPU_Disable(void);
// uint32_t MPU_GetRegionCount(void);

// #endif /* __MPU_H */



/**
 ****************************************************************************************************
 * @file        mpu.h
 * @version     V1.0
 * @brief       MPU内存保护 驱动代码
 ****************************************************************************************************
 * @attention   lyf
 *
 * 实验平台:    GD32H757ZMT6
 *
 ****************************************************************************************************
 */

#ifndef __MPU_H
#define __MPU_H

#include "sys.h"
#include "led.h"
#include "public.h"



void mpu_set_protection(uint32_t baseaddr, uint32_t size, uint32_t rnum, uint8_t de, uint8_t ap, uint8_t sen, uint8_t cen, uint8_t ben);   /* 设置某个区域的MPU保护 */
void mpu_memory_protection(void);   /* 设置需要保护的存储块 */

#endif