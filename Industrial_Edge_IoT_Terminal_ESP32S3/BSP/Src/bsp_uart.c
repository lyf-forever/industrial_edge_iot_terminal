/**
 * @file bsp_uart.c
 * @brief ESP32S3 板级 UART 驱动实现
 *
 * 使用 ESP-IDF UART 驱动 API，配置 UART1 与 GD32H7 进行串口通信。
 * RX 采用事件队列模式，收到数据后通过回调将字节流交给上层协议解析器。
 */

#include "bsp_uart.h"

#if LinkUse

#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static const char *TAG = "bsp_uart";

static QueueHandle_t s_uart_evt_queue = NULL;
static bsp_uart_rx_cb_t s_rx_cb = NULL;
static void *s_rx_cb_user = NULL;

void bsp_uart_init(void *arg)
{
    (void)arg;

    const uart_config_t uart_cfg = {
        .baud_rate  = BSP_UART_BAUDRATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    int intr_alloc_flags = 0;   /* 不强制指定中断优先级 */
    ESP_ERROR_CHECK(uart_driver_install(BSP_UART_PORT, BSP_UART_RX_BUF_SIZE,
                                        BSP_UART_TX_BUF_SIZE,
                                        BSP_UART_EVT_QUEUE_LEN, &s_uart_evt_queue,
                                        intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(BSP_UART_PORT, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(BSP_UART_PORT, BSP_UART_TX_PIN, BSP_UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART%d init OK @%d baud, TX=GPIO%d RX=GPIO%d",
             BSP_UART_PORT, BSP_UART_BAUDRATE, BSP_UART_TX_PIN, BSP_UART_RX_PIN);
}

int bsp_uart_send(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) return 0;
    return uart_write_bytes(BSP_UART_PORT, (const char *)data, len);
}

void bsp_uart_set_rx_callback(bsp_uart_rx_cb_t cb, void *user_data)
{
    s_rx_cb = cb;
    s_rx_cb_user = user_data;
}

void bsp_uart_rx_task(void *arg)
{
    (void)arg;
    uart_event_t evt;
    uint8_t rx_buf[BSP_UART_RX_CHUNK];

    ESP_LOGI(TAG, "RX task started");

    while (1) {
        if (xQueueReceive(s_uart_evt_queue, (void *)&evt, portMAX_DELAY)) {
            switch (evt.type) {
                case UART_DATA:
                {
                    int len = uart_read_bytes(BSP_UART_PORT, rx_buf,
                                              BSP_UART_RX_CHUNK, 0);
                    if (len > 0 && s_rx_cb != NULL) {
                        s_rx_cb(rx_buf, (uint16_t)len, s_rx_cb_user);
                    }
                    break;
                }
                case UART_BREAK:
                    ESP_LOGW(TAG, "UART break detected");
                    break;
                case UART_FRAME_ERR:
                    ESP_LOGW(TAG, "UART frame error");
                    uart_flush_input(BSP_UART_PORT);
                    break;
                case UART_PARITY_ERR:
                    ESP_LOGW(TAG, "UART parity error");
                    uart_flush_input(BSP_UART_PORT);
                    break;
                default:
                    break;
            }
        }
    }
}

#endif /* LinkUse */