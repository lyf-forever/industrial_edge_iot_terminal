#ifndef LINK_PROTOCOL_H_
#define LINK_PROTOCOL_H_

#include "crc.h"
#include <stdint.h>
#include <stdbool.h>

/* 帧常量定义 */
#define LINK_HEADER_0      0xAA
#define LINK_HEADER_1      0x55
#define LINK_TAIL_0        0x0D
#define LINK_TAIL_1        0x0A

#define LINK_MAX_DATA_LEN  12
#define LINK_FRAME_OVERHEAD 9  // header[2] + addr + cmd + data_len + crc16 + tail[2] = 9

/* 命令字（与 ESP32S3 端对齐） */
typedef enum {
    LINK_CMD_HEARTBEAT   = 0x00,   /* 心跳 */
    LINK_CMD_SENSOR_DATA = 0x01,   /* 传感器数据上报 (GD32H7 -> ESP32S3) */
    LINK_CMD_ALARM       = 0x02,   /* 报警事件 */
    LINK_CMD_STATUS_REQ  = 0x03,   /* 状态查询 */
    LINK_CMD_STATUS_RSP  = 0x04,   /* 状态回复 */
    LINK_CMD_LED_CTRL    = 0x10,   /* LED 控制 (ESP32S3 -> GD32H7) */
    LINK_CMD_DISPLAY     = 0x11,   /* 显示更新 */
    LINK_CMD_REBOOT      = 0x12,   /* 远程重启 */
    LINK_CMD_EVENT       = 0x20,   /* 跨核事件帧：data = event_payload */
    LINK_CMD_TIME_SYNC   = 0x21    /* 跨核时间同步（v2.0 补齐，与 ESP32S3 对齐） */
} link_cmd_t;

/* 串口链路帧结构 */
// 发送时构建帧（紧凑）
typedef struct __attribute__((packed)) {
    uint8_t  header[2];
    uint8_t  addr;
    uint8_t  cmd;
    uint8_t  data_len;
    uint8_t  data[LINK_MAX_DATA_LEN];    // 最大预留
    uint16_t crc16;
    uint8_t  tail[2];
} tx_frame_t;

// 接收帧解析状态
typedef enum {
    LINK_STATE_IDLE = 0U,
    LINK_STATE_HEADER1,
    LINK_STATE_HEADER2,
    LINK_STATE_ADDR,
    LINK_STATE_CMD,
    LINK_STATE_DATA_LEN,
    LINK_STATE_DATA,
    LINK_STATE_DATA_ESC,        // 转义序列第二字节（0xCC 前缀解码）
    LINK_STATE_CRC1,
    LINK_STATE_CRC2,
    LINK_STATE_TAIL1,
    LINK_STATE_TAIL2,
    LINK_STATE_COMPLETE
} link_parse_state_t;

// 接收帧上下文
typedef struct {
    link_parse_state_t state;
    uint8_t addr;
    uint8_t cmd;
    uint8_t data_len;
    uint8_t data[LINK_MAX_DATA_LEN];
    uint16_t crc_calc;
    uint16_t crc_recv;
    uint8_t data_index;
} link_parse_ctx_t;

/* 字节转义/解转义函数 */
uint16_t byte_stuff(const uint8_t *src, uint16_t src_len, uint8_t *dst);
uint16_t byte_unstuff(const uint8_t *src, uint16_t src_len, uint8_t *dst);

/* CRC函数 */
uint16_t crc16_modbus(const uint8_t *data, uint16_t len);

/* 帧构建函数 */
uint16_t build_frame(uint8_t addr, uint8_t cmd, const uint8_t *data, uint8_t data_len, uint8_t *frame_buf, uint16_t buf_size);
bool validate_frame(const uint8_t *frame_data, uint16_t frame_len, uint8_t *addr, uint8_t *cmd, uint8_t *data, uint8_t *data_len);

/* 帧解析状态机 */
void link_parse_init(link_parse_ctx_t *ctx);
bool link_parse_byte(link_parse_ctx_t *ctx, uint8_t byte);
bool link_parse_buffer(link_parse_ctx_t *ctx, const uint8_t *buffer, uint16_t len);

/* 帧接收回调（解析到完整帧时调用，跨核事件桥接注册） */
typedef bool (*link_frame_cb_t)(uint8_t addr, uint8_t cmd,
                                const uint8_t *data, uint8_t data_len,
                                void *user_data);
void link_set_frame_callback(link_frame_cb_t cb, void *user_data);

#endif /* LINK_PROTOCOL_H_ */