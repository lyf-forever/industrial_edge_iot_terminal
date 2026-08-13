/*----------------------------------------------*
 *      包含的头文件
 *----------------------------------------------*/
#include "bsp_ws2812.h"

#if Ws2812Use
#include <math.h>
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
#include "esp_log.h" 
#include "driver/rmt_tx.h"

static const char *TAG = "ws2812";
static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];

static const rmt_symbol_word_t ws2812_zero = {
    .level0 = 1,
    .duration0 = 0.3 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T0H=0.3us
    .level1 = 0,
    .duration1 = 0.9 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T0L=0.9us
};
static const rmt_symbol_word_t ws2812_one = {
    .level0 = 1,
    .duration0 = 0.9 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T1H=0.9us
    .level1 = 0,
    .duration1 = 0.3 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T1L=0.3us
};

//reset defaults to 50uS
static const rmt_symbol_word_t ws2812_reset = {
    .level0 = 0,
    .duration0 = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
    .level1 = 0,
    .duration1 = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
};

rmt_channel_handle_t rgbLed_channel = NULL;
rmt_encoder_handle_t simple_encoder = NULL;
rmt_transmit_config_t tx_config = {
    .loop_count = 0, // no transfer loop
};
float offset = 0;
/*----------------------------------*
 *      函数实现
 *----------------------------------------------*/

static size_t encoder_callback(const void *data, size_t data_size,
                               size_t symbols_written, size_t symbols_free,
                               rmt_symbol_word_t *symbols, bool *done, void *arg)
{
    // We need a minimum of 8 symbol spaces to encode a byte. We only
    // need one to encode a reset, but it's simpler to simply demand that
    // there are 8 symbol spaces free to write anything.
    if (symbols_free < 8) {
        return 0;
    }

    // We can calculate where in the data we are from the symbol pos.
    // Alternatively, we could use some counter referenced by the arg
    // parameter to keep track of this.
    size_t data_pos = symbols_written / 8;
    uint8_t *data_bytes = (uint8_t*)data;
    if (data_pos < data_size) {
        // Encode a byte
        size_t symbol_pos = 0;
        for (int bitmask = 0x80; bitmask != 0; bitmask >>= 1) {
            if (data_bytes[data_pos]&bitmask) {
                symbols[symbol_pos++] = ws2812_one;
            } else {
                symbols[symbol_pos++] = ws2812_zero;
            }
        }
        // We're done; we should have written 8 symbols.
        return symbol_pos;
    } else {
        //All bytes already are encoded.
        //Encode the reset, and we're done.
        symbols[0] = ws2812_reset;
        *done = 1; //Indicate end of the transaction.
        return 1; //we only wrote one symbol
    }
}

void ws2812_init(void *arg)
{
    ESP_LOGI(TAG, "Create RMT TX channel");

    rmt_tx_channel_config_t tx_ch_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_ch_cfg, &rgbLed_channel));

    ESP_LOGI(TAG, "Create simple callback-based encoder");

    const rmt_simple_encoder_config_t simple_encoder_cfg = {
        .callback = encoder_callback
        //Note we don't set min_chunk_size here as the default of 64 is good enough.
    };
    ESP_ERROR_CHECK(rmt_new_simple_encoder(&simple_encoder_cfg, &simple_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(rgbLed_channel));
}

//建议20ms调用一次
void ws2812_writeRainbow()
{
    for (int led = 0; led < EXAMPLE_LED_NUMBERS; led++) {
        // Build RGB pixels. Each color is an offset sine, which gives a
        // hue-like effect.
        float angle = offset + (led * EXAMPLE_ANGLE_INC_LED);
        const float color_off = (M_PI * 2) / 3;
        led_strip_pixels[led * 3 + 0] = sin(angle + color_off * 0) * 2 + 1;//127+128
        led_strip_pixels[led * 3 + 1] = sin(angle + color_off * 1) * 2 + 1;//127+128
        led_strip_pixels[led * 3 + 2] = sin(angle + color_off * 2) * 1 + 1;//117+128
    }
    // Flush RGB values to LEDs
    ESP_ERROR_CHECK(rmt_transmit(rgbLed_channel, simple_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(rgbLed_channel, portMAX_DELAY));
    // vTaskDelay(pdMS_TO_TICKS(EXAMPLE_FRAME_DURATION_MS));    //不能在此延时
    //Increase offset to shift pattern
    offset += EXAMPLE_ANGLE_INC_FRAME;
    if (offset > 2 * M_PI) {
        offset -= 2 * M_PI;
    }
}

void ws2812_writeGRB(uint8_t g, uint8_t r, uint8_t b)
{
    for (int led = 0; led < EXAMPLE_LED_NUMBERS; led++) {
        led_strip_pixels[led * 3 + 0] = g;
        led_strip_pixels[led * 3 + 1] = r;
        led_strip_pixels[led * 3 + 2] = b;
    }
    // Flush RGB values to LEDs
    ESP_ERROR_CHECK(rmt_transmit(rgbLed_channel, simple_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(rgbLed_channel, portMAX_DELAY));
}

void ws2812_on(void)
{
    ws2812_writeGRB(3, 18, 12);    // 骚粉色
}

void ws2812_off(void)
{
    ws2812_writeGRB(0, 0, 0);
}

#endif /* Ws2812Use */

