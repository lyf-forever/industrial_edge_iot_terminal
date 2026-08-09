// #include "log_ringbuf.h"
// #include <stdarg.h>
// #include <stdio.h>
// #include <string.h>

// log_ringbuf_t g_log_rb;

// void log_ringbuf_init(uint8_t *buf, uint32_t size, bool overwrite) {
//     g_log_rb.buffer    = buf;
//     g_log_rb.size      = size;
//     g_log_rb.write_idx = 0;
//     g_log_rb.read_idx  = 0;
//     g_log_rb.lost_cnt  = 0;
//     g_log_rb.overwrite = overwrite;
// }

// static uint32_t log_ringbuf_space(void) {
//     uint32_t w = g_log_rb.write_idx;
//     uint32_t r = g_log_rb.read_idx;
//     if (w >= r) return (g_log_rb.size - (w - r));
//     else        return (r - w);
// }

// static uint32_t log_ringbuf_write(uint8_t *data, uint32_t len) {
//     uint32_t w = g_log_rb.write_idx;
//     uint32_t r = g_log_rb.read_idx;
//     uint32_t free_space = (w >= r) ? (g_log_rb.size - (w - r)) : (r - w);
    
//     if (len > free_space) {
//         if (g_log_rb.overwrite) {
//             // 覆盖模式：移动读指针，丢弃最旧数据
//             uint32_t need = len - free_space;
//             if (need >= g_log_rb.size) {
//                 // 极端情况：整个缓冲区都不够一条日志，直接丢弃
//                 g_log_rb.lost_cnt++;
//                 return 0;
//             }
//             g_log_rb.read_idx = (r + need) % g_log_rb.size;
//         } else {
//             g_log_rb.lost_cnt++;
//             return 0;  // 丢弃新日志
//         }
//     }
    
//     // 实际写入（处理环形回绕）
//     uint32_t first_chunk = g_log_rb.size - w;
//     if (first_chunk > len) first_chunk = len;
//     memcpy(&g_log_rb.buffer[w], data, first_chunk);
//     if (len > first_chunk) {
//         memcpy(g_log_rb.buffer, data + first_chunk, len - first_chunk);
//     }
//     g_log_rb.write_idx = (w + len) % g_log_rb.size;
//     return len;
// }

// void log_printf(uint8_t level, const char *fmt, ...) {
//     va_list args;
//     // 1. 计算格式化后的字符串长度
//     va_start(args, fmt);
//     int msg_len = vsnprintf(NULL, 0, fmt, args);
//     va_end(args);
//     if (msg_len <= 0) return;
    
//     // 2. 分配临时缓冲区（栈上），也可以直接写入环形缓冲区，但为了简单，先拼装
//     uint32_t total_len = sizeof(log_entry_hdr_t) + msg_len + 1; // +1 for '\0'
//     uint8_t *temp_buf = (uint8_t*)__builtin_alloca(total_len);  // 栈上分配
//     if (!temp_buf) return;
    
//     log_entry_hdr_t *hdr = (log_entry_hdr_t*)temp_buf;
//     hdr->timestamp = get_system_tick();   // 需要用户实现
//     hdr->line      = 0;                   // 可通过宏传递 __LINE__
//     hdr->level     = level;
//     hdr->len       = msg_len;
    
//     char *msg_start = (char*)(temp_buf + sizeof(log_entry_hdr_t));
//     va_start(args, fmt);
//     vsnprintf(msg_start, msg_len + 1, fmt, args);
//     va_end(args);
    
//     // 3. 写入环形缓冲区
//     log_ringbuf_write(temp_buf, total_len);
// }

// // 导出所有日志到调试接口（例如 USB CDC 或 SWO）
// void log_export_all(void) {
//     uint32_t r = g_log_rb.read_idx;
//     uint32_t w = g_log_rb.write_idx;
//     while (r != w) {
//         // 读取一条日志的头部
//         log_entry_hdr_t *hdr = (log_entry_hdr_t*)&g_log_rb.buffer[r];
//         uint32_t entry_len = sizeof(log_entry_hdr_t) + hdr->len + 1;
//         // 通过用户定义的低速发送函数（如 usb_printf）发送
//         // 为避免阻塞，可以分块发送或放入另一个队列
//         usb_printf("[%u][%u] %s\n", hdr->timestamp, hdr->level, 
//                    (char*)hdr + sizeof(log_entry_hdr_t));
//         r = (r + entry_len) % g_log_rb.size;
//         g_log_rb.read_idx = r;  // 更新已读指针
//     }
// }