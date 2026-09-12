#ifndef __LINK_PROTOCOL_H_
#define __LINK_PROTOCOL_H_

/**
 * @file link_protocol.h
 * @brief 串口链路帧协议（纯协议层，无硬件耦合）
 *
 * 作用与 GD32H7 端 Drivers/BSP/Inc/link_protocol.h 完全一致，
 * 是双 MCU 之间通信的帧格式定义与编解码实现。两端必须保持相同的
 * 帧常量、帧结构、CRC 算法与字节转义规则。
 *
 * 与 GD32H7 端的区别：本版本去除了对 LCD/GPIO 的直接耦合，
 * 帧解析成功后通过注册的回调函数通知上层（传输介质无关），
 * 从而可复用于 UART / BLE / 任意字节流传输。
 *
 * 帧格式（解转义后）：
 *   [0xAA][0x55][addr][cmd][data_len][data[0..data_len-1]][crc_hi][crc_lo][0x0D][0x0A]
 *   overhead = 8 字节，data 最多 12 字节
 *
 * CRC: CRC-16/Modbus，init 0xFFFF，反射多项式 0xA001，线上大端(高字节先发)
 *
 * 字节转义（仅对 data 区；addr/cmd/data_len/CRC 不转义）：
 *   0xAA -> 0xCC 0x01, 0x55 -> 0xCC 0x02,
 *   0x0D -> 0xCC 0x03, 0x0A -> 0xCC 0x04, 0xCC -> 0xCC 0xCC
 */

#include <stdint.h>
#include <stdbool.h>

/* 帧常量定义（与 GD32H7 端完全一致） */
#define LINK_HEADER_0       0xAA
#define LINK_HEADER_1       0x55
#define LINK_TAIL_0         0x0D
#define LINK_TAIL_1         0x0A

#define LINK_MAX_DATA_LEN   12
#define LINK_FRAME_OVERHEAD 8   /* header[2] + addr + cmd + data_len + crc16[2] + tail[2] */

/* ===================== 帧结构（紧凑） ============================== */
#pragma pack(push, 1)
/* 发送时构建帧（紧凑，与 GD32H7 端 tx_frame_t 布局一致） */
typedef struct __attribute__((packed)) {
    uint8_t  header[2];
    uint8_t  addr;
    uint8_t  cmd;
    uint8_t  data_len;
    uint8_t  data[LINK_MAX_DATA_LEN];   /* 最大预留 */
    uint16_t crc16;
    uint8_t  tail[2];
} tx_frame_t;
#pragma pack(pop)

/* ===================== 接收帧解析状态 =============================== */
typedef enum {
    LINK_STATE_IDLE = 0U,
    LINK_STATE_HEADER1,
    LINK_STATE_HEADER2,
    LINK_STATE_ADDR,
    LINK_STATE_CMD,
    LINK_STATE_DATA_LEN,
    LINK_STATE_DATA,
    LINK_STATE_DATA_ESC,        /* 转义序列第二字节（0xCC 前缀解码） */
    LINK_STATE_CRC1,
    LINK_STATE_CRC2,
    LINK_STATE_TAIL1,
    LINK_STATE_TAIL2,
    LINK_STATE_COMPLETE
} link_parse_state_t;

/* 接收帧解析上下文 */
typedef struct {
    link_parse_state_t state;
    uint8_t  addr;
    uint8_t  cmd;
    uint8_t  data_len;
    uint8_t  data[LINK_MAX_DATA_LEN];
    uint16_t crc_calc;
    uint16_t crc_recv;
    uint8_t  data_index;
} link_parse_ctx_t;

/* ===================== 帧接收回调 =============================== *
 * 解析到完整且 CRC 正确的帧时调用。上层可借此将帧分发到业务逻辑。
 * 回调返回值目前未使用，保留以供未来流控扩展。
 * */
typedef bool (*link_frame_cb_t)(uint8_t addr, uint8_t cmd,
                                const uint8_t *data, uint8_t data_len,
                                void *user_data);

/* ===================== API 声明 =============================== */

/* CRC 函数（与 GD32H7 端 crc16_modbus 完全一致，可独立复用） */
uint16_t crc16_modbus(const uint8_t *data, uint16_t len);

/* 字节转义/解转义函数 */
uint16_t byte_stuff(const uint8_t *src, uint16_t src_len, uint8_t *dst);
uint16_t byte_unstuff(const uint8_t *src, uint16_t src_len, uint8_t *dst);

/* 帧构建函数：返回帧长度(含转义)，0 表示失败 */
uint16_t build_frame(uint8_t addr, uint8_t cmd, const uint8_t *data,
                     uint8_t data_len, uint8_t *frame_buf, uint16_t buf_size);

/* 帧验证函数（对已解转义的帧数据） */
bool validate_frame(const uint8_t *frame_data, uint16_t frame_len,
                    uint8_t *addr, uint8_t *cmd, uint8_t *data, uint8_t *data_len);

/* 帧解析状态机 */
void link_parse_init(link_parse_ctx_t *ctx);
bool link_parse_byte(link_parse_ctx_t *ctx, uint8_t byte);
bool link_parse_buffer(link_parse_ctx_t *ctx, const uint8_t *buffer, uint16_t len);

/* 回调注册：解析到完整帧时通知上层 */
void link_set_frame_callback(link_frame_cb_t cb, void *user_data);

#endif /* __LINK_PROTOCOL_H_ */