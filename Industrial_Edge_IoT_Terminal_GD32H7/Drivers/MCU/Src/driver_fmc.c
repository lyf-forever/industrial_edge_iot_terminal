/*!
 * 文件名称： driver_fmc.c
 * 描    述： fmc 驱动文件
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

#include "driver_fmc.h"

/*!
* 说明       向内部Flash写入数据
* 输入[1]    write_start_addr：写入数据的Flash地址  
* 输入[2]    data_buf：        写入数据的buf指针
* 输入[3]    data_lengh：      写入数据的长度
* 返回值     无
*/
void fmc_write_data(uint32_t write_start_addr, uint8_t *data_buf, uint16_t data_lengh)
{
		uint32_t write_addr,erase_addr;
		uint16_t data_write_num=0,data_lengh_word=data_lengh/4;
		int32_t data_earse_num;
        uint32_t* pbuff=(uint32_t*)data_buf;
    
        if(data_lengh%4 !=0)
        {
            data_lengh_word++;        
        }
        
        
		fmc_unlock();													/* 解锁FMC */
		/* 清除错误标志 */
		fmc_flag_clear(FMC_FLAG_WPERR);
		fmc_flag_clear(FMC_FLAG_PGSERR);		
		fmc_flag_clear(FMC_FLAG_RPERR);
		fmc_flag_clear(FMC_FLAG_RSERR);
		fmc_flag_clear(FMC_FLAG_ECCCOR);
        fmc_flag_clear(FMC_FLAG_ECCDET);
        fmc_flag_clear(FMC_FLAG_OBMERR);
        
		erase_addr = write_start_addr;
		data_earse_num = data_lengh;

        if(write_start_addr%FLAG_PAGE_SIZE == 0)  /* 若写入地址为页起始地址 */
        {
            for(;data_earse_num>0;)
            {
                fmc_sector_erase(erase_addr);
                /* 清除错误标志 */
                fmc_flag_clear(FMC_FLAG_WPERR);
                fmc_flag_clear(FMC_FLAG_PGSERR);		
                fmc_flag_clear(FMC_FLAG_RPERR);
                fmc_flag_clear(FMC_FLAG_RSERR);
                fmc_flag_clear(FMC_FLAG_ECCCOR);
                fmc_flag_clear(FMC_FLAG_ECCDET);
                fmc_flag_clear(FMC_FLAG_OBMERR);
                erase_addr+=FLAG_PAGE_SIZE;
                data_earse_num-=FLAG_PAGE_SIZE;
            }
        }else{
                /*若写入地址不是页起始地址*/
                for(;(data_earse_num>0||erase_addr>=write_start_addr+data_lengh);)
                {
                    fmc_sector_erase(erase_addr);
                    /* 清除错误标志 */
                    fmc_flag_clear(FMC_FLAG_WPERR);
                    fmc_flag_clear(FMC_FLAG_PGSERR);		
                    fmc_flag_clear(FMC_FLAG_RPERR);
                    fmc_flag_clear(FMC_FLAG_RSERR);
                    fmc_flag_clear(FMC_FLAG_ECCCOR);
                    fmc_flag_clear(FMC_FLAG_ECCDET);
                    fmc_flag_clear(FMC_FLAG_OBMERR);
                    erase_addr+=FLAG_PAGE_SIZE;
                    data_earse_num-=FLAG_PAGE_SIZE;
                }
        }
		
		/* 写入数据 */
		write_addr = write_start_addr;

		for(data_write_num = 0; data_write_num<data_lengh_word;data_write_num++)
		{
			fmc_word_program(write_addr, pbuff[data_write_num]);
            /* 清除错误标志 */
            fmc_flag_clear(FMC_FLAG_WPERR);
            fmc_flag_clear(FMC_FLAG_PGSERR);		
            fmc_flag_clear(FMC_FLAG_RPERR);
            fmc_flag_clear(FMC_FLAG_RSERR);
            fmc_flag_clear(FMC_FLAG_ECCCOR);
            fmc_flag_clear(FMC_FLAG_ECCDET);
            fmc_flag_clear(FMC_FLAG_OBMERR);
                       
			write_addr+=4;
		}
		fmc_lock();
}

/*!
* 说明       读取Flash地址数据
* 输入[1]    write_read_addr：需要读取的Flash地址
* 返回值     读取的数据
*/
uint8_t fmc_read_data(uint32_t write_read_addr)
{
		return *(uint8_t *)write_read_addr;
}

