#include "sys.h"
#include "esp_rom_sys.h"

/* 延时/us */
inline void delay_us(uint32_t us)
{
    esp_rom_delay_us(us);
}

