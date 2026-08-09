// #include "log_ringbuf.h"

// // 假设你已经实现了 USB 虚拟串口驱动
// extern void usb_cdc_send(const uint8_t *data, uint32_t len);

// // 低优先级任务，每隔 10ms 尝试导出
// void log_export_task(void *arg) {
//     while (1) {
//         uint32_t r = g_log_rb.read_idx;
//         uint32_t w = g_log_rb.write_idx;
//         if (r != w) {
//             // 一次最多发送 256 字节，避免长时间占用 USB
//             uint32_t chunk = (w > r) ? (w - r) : (g_log_rb.size - r);
//             if (chunk > 256) chunk = 256;
//             usb_cdc_send(&g_log_rb.buffer[r], chunk);
//             r = (r + chunk) % g_log_rb.size;
//             g_log_rb.read_idx = r;
//         }
//         vTaskDelay(pdMS_TO_TICKS(10));
//     }
// }