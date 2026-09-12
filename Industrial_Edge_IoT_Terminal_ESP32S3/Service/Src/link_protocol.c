/**
 * @file link_protocol.c
 * @brief 串口链路帧协议纯实现（从 GD32H7 端移植，去除硬件耦合）
 *
 * 与 GD32H7 端 link_protocol.c 的算法完全一致，但：
 *  1. 移除对 bsp_8080_lcd / led GPIO 的直接耦合；
 *  2. 帧解析成功改为通过注册的回调通知上层，便于复用于任意传输介质；
 *  3. CRC 实现合并为本文件内独立的 crc16_modbus，无需外部 crc.c。
 */

#include "link_protocol.h"
#include <string.h>

/* 帧接收回调（默认为 NULL，由上层注册） */
static link_frame_cb_t s_frame_cb = NULL;
static void           *s_frame_cb_user = NULL;

/* ===================== CRC-16/Modbus =============================== *
 * init 0xFFFF，反射多项式 0xA001(= 0x8005 反转)。
 * 与 GD32H7 端 crc.c 的 crc16_modbus 算法逐位一致。
 * */
uint16_t crc16_modbus(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/* ===================== 字节转义/解转义 ============================= *
 * 规则与 GD32H7 端 link_protocol.c 完全一致。
 * */
uint16_t byte_stuff(const uint8_t *src, uint16_t src_len, uint8_t *dst)
{
    uint16_t dst_len = 0;
    for (uint16_t i = 0; i < src_len; i++) {
        uint8_t byte = src[i];
        switch (byte) {
            case 0xAA: dst[dst_len++] = 0xCC; dst[dst_len++] = 0x01; break;
            case 0x55: dst[dst_len++] = 0xCC; dst[dst_len++] = 0x02; break;
            case 0x0D: dst[dst_len++] = 0xCC; dst[dst_len++] = 0x03; break;
            case 0x0A: dst[dst_len++] = 0xCC; dst[dst_len++] = 0x04; break;
            case 0xCC: dst[dst_len++] = 0xCC; dst[dst_len++] = 0xCC; break;
            default:   dst[dst_len++] = byte; break;
        }
    }
    return dst_len;
}

uint16_t byte_unstuff(const uint8_t *src, uint16_t src_len, uint8_t *dst)
{
    uint16_t dst_len = 0;
    uint16_t i = 0;
    while (i < src_len) {
        uint8_t byte = src[i++];
        if (byte == 0xCC) {
            if (i >= src_len) return 0;   /* 转义序列不完整 */
            uint8_t next = src[i++];
            switch (next) {
                case 0x01: dst[dst_len++] = 0xAA; break;
                case 0x02: dst[dst_len++] = 0x55; break;
                case 0x03: dst[dst_len++] = 0x0D; break;
                case 0x04: dst[dst_len++] = 0x0A; break;
                case 0xCC: dst[dst_len++] = 0xCC; break;
                default:   return 0;     /* 非法转义序列 */
            }
        } else {
            dst[dst_len++] = byte;
        }
    }
    return dst_len;
}

/* ===================== 帧构建 ============================== *
 * 与 GD32H7 端 build_frame 算法一致：构造原始帧 -> 计算 CRC
 * (addr|cmd|data_len|data) -> 转义中间区段 -> 拼帧头帧尾输出。
 * */
uint16_t build_frame(uint8_t addr, uint8_t cmd, const uint8_t *data,
                     uint8_t data_len, uint8_t *frame_buf, uint16_t buf_size)
{
    if (data_len > LINK_MAX_DATA_LEN) return 0;
    if (frame_buf == NULL) return 0;

    /* CRC 覆盖 addr + cmd + data_len + data（未转义的原始值） */
    uint8_t crc_data[3 + LINK_MAX_DATA_LEN];
    uint8_t crc_len = 0;
    crc_data[crc_len++] = addr;
    crc_data[crc_len++] = cmd;
    crc_data[crc_len++] = data_len;
    if (data_len > 0 && data != NULL) {
        memcpy(&crc_data[crc_len], data, data_len);
        crc_len += data_len;
    }
    uint16_t crc = crc16_modbus(crc_data, crc_len);

    /* 长度上界：帧头2 + addr/cmd/len 3 + data 最坏2倍 + crc 2 + 帧尾2 */
    uint16_t need = (uint16_t)(2 + 3 + (uint16_t)(2 * data_len) + 2 + 2);
    if (need > buf_size) return 0;

    /* 线上帧：转义仅作用于 data 区；CRC 线上大端（高字节先发） */
    uint16_t i = 0;
    frame_buf[i++] = LINK_HEADER_0;
    frame_buf[i++] = LINK_HEADER_1;
    frame_buf[i++] = addr;
    frame_buf[i++] = cmd;
    frame_buf[i++] = data_len;
    if (data_len > 0 && data != NULL) {
        i += byte_stuff(data, data_len, &frame_buf[i]);
    }
    frame_buf[i++] = (uint8_t)(crc >> 8);
    frame_buf[i++] = (uint8_t)(crc & 0xFF);
    frame_buf[i++] = LINK_TAIL_0;
    frame_buf[i++] = LINK_TAIL_1;

    return i;
}

/* ===================== 帧验证（对已解转义帧） ====================== *
 * 与 GD32H7 端 validate_frame 算法一致。
 * */
bool validate_frame(const uint8_t *frame_data, uint16_t frame_len,
                    uint8_t *addr, uint8_t *cmd, uint8_t *data, uint8_t *data_len)
{
    if (frame_len < LINK_FRAME_OVERHEAD) return false;

    if (frame_data[0] != LINK_HEADER_0 || frame_data[1] != LINK_HEADER_1) return false;
    if (frame_data[frame_len - 2] != LINK_TAIL_0 || frame_data[frame_len - 1] != LINK_TAIL_1) return false;

    *addr     = frame_data[2];
    *cmd      = frame_data[3];
    *data_len = frame_data[4];

    if (*data_len > LINK_MAX_DATA_LEN) return false;
    if (frame_len != (LINK_FRAME_OVERHEAD + *data_len)) return false;

    if (*data_len > 0 && data != NULL) {
        memcpy(data, &frame_data[5], *data_len);
    }

    uint8_t crc_data[3 + LINK_MAX_DATA_LEN];
    uint8_t crc_len = 0;
    crc_data[crc_len++] = *addr;
    crc_data[crc_len++] = *cmd;
    crc_data[crc_len++] = *data_len;
    if (*data_len > 0) {
        memcpy(&crc_data[crc_len], data, *data_len);
        crc_len += *data_len;
    }
    uint16_t crc_calc = crc16_modbus(crc_data, crc_len);
    uint16_t crc_recv = ((uint16_t)frame_data[frame_len - 4] << 8) | (uint16_t)frame_data[frame_len - 3];

    return crc_calc == crc_recv;
}

/* ===================== 帧解析状态机 =============================== *
 * 与 GD32H7 端 link_parse_byte 状态机一致，但解析成功后
 * 通过注册的回调通知上层，而非直接操作 LCD/GPIO。
 * */
void link_parse_init(link_parse_ctx_t *ctx)
{
    if (ctx == NULL) return;
    memset(ctx, 0, sizeof(link_parse_ctx_t));
    ctx->state = LINK_STATE_IDLE;
}

bool link_parse_byte(link_parse_ctx_t *ctx, uint8_t byte)
{
    if (ctx == NULL) return false;

    switch (ctx->state) {
        case LINK_STATE_IDLE:
            if (byte == LINK_HEADER_0) ctx->state = LINK_STATE_HEADER1;
            break;

        case LINK_STATE_HEADER1:
            ctx->state = (byte == LINK_HEADER_1) ? LINK_STATE_ADDR : LINK_STATE_IDLE;
            break;

        case LINK_STATE_ADDR:
            ctx->addr = byte;
            ctx->state = LINK_STATE_CMD;
            break;

        case LINK_STATE_CMD:
            ctx->cmd = byte;
            ctx->state = LINK_STATE_DATA_LEN;
            break;

        case LINK_STATE_DATA_LEN:
            if (byte <= LINK_MAX_DATA_LEN) {
                ctx->data_len = byte;
                ctx->data_index = 0;
                ctx->state = (ctx->data_len > 0) ? LINK_STATE_DATA : LINK_STATE_CRC1;
            } else {
                ctx->state = LINK_STATE_IDLE;
            }
            break;

        case LINK_STATE_DATA:
            /* 支持字节转义：0xCC 前缀进入转义解码（与 GD32H7 端一致） */
            if (byte == 0xCC) {
                ctx->state = LINK_STATE_DATA_ESC;
            } else {
                ctx->data[ctx->data_index++] = byte;
                if (ctx->data_index >= ctx->data_len) ctx->state = LINK_STATE_CRC1;
            }
            break;

        case LINK_STATE_DATA_ESC:
            if (byte == 0x01)      ctx->data[ctx->data_index++] = 0xAA;
            else if (byte == 0x02) ctx->data[ctx->data_index++] = 0x55;
            else if (byte == 0x03) ctx->data[ctx->data_index++] = 0x0D;
            else if (byte == 0x04) ctx->data[ctx->data_index++] = 0x0A;
            else if (byte == 0xCC) ctx->data[ctx->data_index++] = 0xCC;
            else {
                ctx->state = LINK_STATE_IDLE;   /* 非法转义：丢弃整帧 */
                break;
            }
            ctx->state = (ctx->data_index >= ctx->data_len) ? LINK_STATE_CRC1 : LINK_STATE_DATA;
            break;

        case LINK_STATE_CRC1:
            ctx->crc_recv = ((uint16_t)byte) << 8;
            ctx->state = LINK_STATE_CRC2;
            break;

        case LINK_STATE_CRC2:
            ctx->crc_recv |= (uint16_t)byte;
            ctx->state = LINK_STATE_TAIL1;
            break;

        case LINK_STATE_TAIL1:
            ctx->state = (byte == LINK_TAIL_0) ? LINK_STATE_TAIL2 : LINK_STATE_IDLE;
            break;

        case LINK_STATE_TAIL2:
            if (byte == LINK_TAIL_1) {
                uint8_t crcdata[3 + ctx->data_len];
                crcdata[0] = ctx->addr;
                crcdata[1] = ctx->cmd;
                crcdata[2] = ctx->data_len;
                if (ctx->data_len > 0) memcpy(&crcdata[3], ctx->data, ctx->data_len);
                ctx->crc_calc = crc16_modbus(crcdata, 3 + ctx->data_len);

                if (ctx->crc_calc == ctx->crc_recv) {
                    /* 帧完整且 CRC 正确：通知上层回调 */
                    if (s_frame_cb != NULL) {
                        s_frame_cb(ctx->addr, ctx->cmd, ctx->data, ctx->data_len, s_frame_cb_user);
                    }
                    ctx->state = LINK_STATE_IDLE;
                    return true;
                } else {
                    ctx->state = LINK_STATE_IDLE;
                    return false;
                }
            } else {
                ctx->state = LINK_STATE_IDLE;
            }
            break;

        default:
            ctx->state = LINK_STATE_IDLE;
            break;
    }
    return false;
}

bool link_parse_buffer(link_parse_ctx_t *ctx, const uint8_t *buffer, uint16_t len)
{
    if (ctx == NULL || buffer == NULL) return false;
    for (uint16_t i = 0; i < len; i++) {
        if (link_parse_byte(ctx, buffer[i])) {
            return true;
        }
    }
    return false;
}

/* ===================== 回调注册 ============================== */
void link_set_frame_callback(link_frame_cb_t cb, void *user_data)
{
    s_frame_cb = cb;
    s_frame_cb_user = user_data;
}