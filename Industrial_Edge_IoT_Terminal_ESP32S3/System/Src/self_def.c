#include "self_def.h"
#include "driver/gpio.h"

void gpio_toggle_level(gpio_num_t gpio)
{
    gpio_set_level(gpio, !gpio_get_level(gpio));
}


