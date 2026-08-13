#include "link_protocol.h"
#include "gd32h7xx_gpio.h"
#include "led.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* 帧接收回调（默认 NULL，由 event_ipc 注册） */
static link_frame_cb_t s_frame_cb = NULL;
static void *s_frame_cb_user = NULL;

void link_set_frame_callback(link_frame_cb_t cb, void *user_data)
{
    s_frame_cb = cb;
    s_frame_cb_user = user_data;
}

static inline uint16_t crc16_modbus_update(uint16_t crc, uint8_t data)
{
    crc ^= data;
    for (int i = 0; i < 8; i++) {
        if (crc & 1) {
            crc = (crc >> 1) ^ 0xA001;  // 0xA001 = 0x8005 反转后的多项式
        } else {
            crc >>= 1;
        }
    }
    return crc;
}

/**
 * @brief 对数据域进行字节转义
 * @param src      原始数据指针
 * @param src_len  原始数据长度
 * @param dst      输出缓冲区（必须足够大，最大为 src_len * 2）
 * @return 转义后的字节数
 */
uint16_t byte_stuff(const uint8_t *src, uint16_t src_len, uint8_t *dst)
{
    uint16_t dst_len = 0;
    for (uint16_t i = 0; i < src_len; i++) {
        uint8_t byte = src[i];
        switch (byte) {
            case 0xAA:
                dst[dst_len++] = 0xCC;
                dst[dst_len++] = 0x01;
                break;
            case 0x55:
                dst[dst_len++] = 0xCC;
                dst[dst_len++] = 0x02;
                break;
            case 0x0D:
                dst[dst_len++] = 0xCC;
                dst[dst_len++] = 0x03;
                break;
            case 0x0A:
                dst[dst_len++] = 0xCC;
                dst[dst_len++] = 0x04;
                break;
            case 0xCC:
                dst[dst_len++] = 0xCC;
                dst[dst_len++] = 0xCC;
                break;
            default:
                dst[dst_len++] = byte;
                break;
        }
    }
    return dst_len;
}

/**
 * @brief 对接收到的数据域进行逆转义
 * @param src      转义后的数据指针
 * @param src_len  转义后的数据长度
 * @param dst      输出缓冲区（长度至少为 src_len）
 * @return 逆转义后的实际字节数，若遇到非法序列返回 0
 */
uint16_t byte_unstuff(const uint8_t *src, uint16_t src_len, uint8_t *dst)
{
    uint16_t dst_len = 0;
    uint16_t i = 0;
    while (i < src_len) {
        uint8_t byte = src[i++];
        if (byte == 0xCC) {
            // 需要下一个字节来确定原值
            if (i >= src_len) return 0; // 错误：转义序列不完整
            uint8_t next = src[i++];
            switch (next) {
                case 0x01: dst[dst_len++] = 0xAA; break;
                case 0x02: dst[dst_len++] = 0x55; break;
                case 0x03: dst[dst_len++] = 0x0D; break;
                case 0x04: dst[dst_len++] = 0x0A; break;
                case 0xCC: dst[dst_len++] = 0xCC; break;
                default: return 0; // 非法转义序列
            }
        } else {
            dst[dst_len++] = byte;
        }
    }
    return dst_len;
}
                     

/**
 * @brief 构建完整的数据帧（包含字节转义）
 * @param addr 设备地址
 * @param cmd 命令字
 * @param data 数据指针
 * @param data_len 数据长度（0-12）
 * @param frame_buf 输出帧缓冲区
 * @param buf_size 缓冲区大小
 * @return 帧长度（字节），0表示失败
 */
uint16_t build_frame(uint8_t addr, uint8_t cmd, const uint8_t *data, uint8_t data_len, uint8_t *frame_buf, uint16_t buf_size)
{
    if (data_len > LINK_MAX_DATA_LEN) {
        return 0;
    }

    // 构建原始帧（未转义）
    tx_frame_t raw_frame;
    raw_frame.header[0] = LINK_HEADER_0;
    raw_frame.header[1] = LINK_HEADER_1;
    raw_frame.addr = addr;
    raw_frame.cmd = cmd;
    raw_frame.data_len = data_len;
    if (data_len > 0 && data != NULL) {
        memcpy(raw_frame.data, data, data_len);
    }

    // 计算CRC（不包括头尾）
    uint8_t crc_data[2 + LINK_MAX_DATA_LEN]; // addr + cmd + data_len + data
    uint8_t crc_len = 0;
    crc_data[crc_len++] = raw_frame.addr;
    crc_data[crc_len++] = raw_frame.cmd;
    crc_data[crc_len++] = raw_frame.data_len;
    if (raw_frame.data_len > 0) {
        memcpy(&crc_data[crc_len], raw_frame.data, raw_frame.data_len);
        crc_len += raw_frame.data_len;
    }
    raw_frame.crc16 = crc16_modbus(crc_data, crc_len);

    raw_frame.tail[0] = LINK_TAIL_0;
    raw_frame.tail[1] = LINK_TAIL_1;

    // 将原始帧转换为字节数组(去除帧头帧尾)
    uint8_t raw_buffer[sizeof(tx_frame_t) - 4U];
    memcpy(raw_buffer, &raw_frame.addr, sizeof(tx_frame_t) - 4U);

    uint8_t frameBuf[2 * (sizeof(tx_frame_t) - 4U)];   // 临时局部数据转义缓冲区
    // 字节转义
    uint16_t escaped_len = byte_stuff(raw_buffer, sizeof(tx_frame_t) - 4U, frameBuf);

    /* 完整的帧输出缓冲区 */
    frame_buf[0] = raw_frame.header[0];
    frame_buf[1] = raw_frame.header[1];
    memcpy(&frame_buf[2], frameBuf, escaped_len);
    frame_buf[2 + escaped_len] = raw_frame.tail[0];
    frame_buf[2 + escaped_len + 1] = raw_frame.tail[1];

    return escaped_len + 4;
}

/**
 * @brief 验证并解析接收到的帧（已解转义）
 * @param frame_data 已解转义的帧数据
 * @param frame_len 帧长度
 * @param addr 输出设备地址
 * @param cmd 输出命令字
 * @param data 输出数据缓冲区
 * @param data_len 输出数据长度
 * @return true: 帧有效，false: 帧无效
 */
bool validate_frame(const uint8_t *frame_data, uint16_t frame_len, uint8_t *addr, uint8_t *cmd, uint8_t *data, uint8_t *data_len)
{
    if (frame_len < LINK_FRAME_OVERHEAD) {
        return false;
    }

    // 检查头尾
    if (frame_data[0] != LINK_HEADER_0 || frame_data[1] != LINK_HEADER_1) {
        return false;
    }

    if (frame_data[frame_len - 2] != LINK_TAIL_0 || frame_data[frame_len - 1] != LINK_TAIL_1) {
        return false;
    }

    // 提取字段
    *addr = frame_data[2];
    *cmd = frame_data[3];
    *data_len = frame_data[4];

    if (*data_len > LINK_MAX_DATA_LEN) {
        return false;
    }

    // 检查数据长度是否匹配
    if (frame_len != (LINK_FRAME_OVERHEAD + *data_len)) {
        return false;
    }

    // 复制数据
    if (*data_len > 0 && data != NULL) {
        memcpy(data, &frame_data[5], *data_len);
    }

    // 验证CRC
    uint8_t crc_data[2 + LINK_MAX_DATA_LEN];
    uint8_t crc_len = 0;
    crc_data[crc_len++] = *addr;
    crc_data[crc_len++] = *cmd;
    crc_data[crc_len++] = *data_len;
    if (*data_len > 0) {
        memcpy(&crc_data[crc_len], data, *data_len);
        crc_len += *data_len;
    }

    uint16_t crc_calc = crc16_modbus(crc_data, crc_len);
    uint16_t crc_recv = (((uint16_t)frame_data[frame_len - 4]) << 8) | ((uint16_t)frame_data[frame_len - 3]);

    if (crc_calc != crc_recv) {
        return false;
    }

    return true;
}

/**
 * @brief 初始化帧解析上下文
 */
void link_parse_init(link_parse_ctx_t *ctx)
{
    if (ctx == NULL) return;

    memset(ctx, 0, sizeof(link_parse_ctx_t));
    ctx->state = LINK_STATE_IDLE;
}

/**
 * @brief 解析单个字节（状态机）
 * @param ctx 解析上下文
 * @param byte 接收到的字节
 * @return true: 帧解析完成，false: 帧未完成或错误
 */
bool link_parse_byte(link_parse_ctx_t *ctx, uint8_t byte)
{
    if (ctx == NULL) return false;

    switch (ctx->state) {
        case LINK_STATE_IDLE:
            if (byte == LINK_HEADER_0) {
                ctx->state = LINK_STATE_HEADER1;
            }
            break;

        case LINK_STATE_HEADER1:
            if (byte == LINK_HEADER_1) {
                ctx->state = LINK_STATE_ADDR;
            } else {
                ctx->state = LINK_STATE_IDLE;
            }
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
                if (ctx->data_len > 0) {
                    ctx->state = LINK_STATE_DATA;
                } else {
                    ctx->state = LINK_STATE_CRC1;
                }
            } else {
                ctx->state = LINK_STATE_IDLE; // 数据长度无效
            }
            break;

        case LINK_STATE_DATA:
            /* 支持字节转义：0xCC 前缀的转义序列在此解码（架构 3.2 双端互通要求） */
            if (byte == 0xCC) {
                ctx->state = LINK_STATE_DATA_ESC;
            } else {
                ctx->data[ctx->data_index++] = byte;
                if (ctx->data_index >= ctx->data_len) {
                    ctx->state = LINK_STATE_CRC1;
                }
            }
            break;

        case LINK_STATE_DATA_ESC:
            /* 转义序列第二个字节：还原原始字节 */
            switch (byte) {
                case 0x01: ctx->data[ctx->data_index++] = 0xAA; break;
                case 0x02: ctx->data[ctx->data_index++] = 0x55; break;
                case 0x03: ctx->data[ctx->data_index++] = 0x0D; break;
                case 0x04: ctx->data[ctx->data_index++] = 0x0A; break;
                case 0xCC: ctx->data[ctx->data_index++] = 0xCC; break;
                default:
                    /* 非法转义序列：丢帧复位 */
                    ctx->state = LINK_STATE_IDLE;
                    break;
            }
            if (ctx->state == LINK_STATE_DATA_ESC) {
                ctx->state = LINK_STATE_DATA;
                if (ctx->data_index >= ctx->data_len) {
                    ctx->state = LINK_STATE_CRC1;
                }
            }
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
            if (byte == LINK_TAIL_0) {
                ctx->state = LINK_STATE_TAIL2;
            } else {
                ctx->state = LINK_STATE_IDLE;
            }
            break;

        case LINK_STATE_TAIL2:
            if (byte == LINK_TAIL_1) {
                // 验证CRC
                uint8_t crcdata[3 + ctx->data_len];
                crcdata[0] = ctx->addr;
                crcdata[1] = ctx->cmd;
                crcdata[2] = ctx->data_len;
                memcpy(&crcdata[3], ctx->data, ctx->data_len);
                ctx->crc_calc = crc16_modbus(crcdata, 3 + ctx->data_len);
                /* v2.0：移除每帧 LCD 刷屏调试残留（bsp_8080_lcd_clear/printf
                 * 每次收帧全屏刷新，拖慢解析且干扰显示） */
                if(ctx->crc_calc == ctx->crc_recv) {
                    gpio_bit_toggle(LED1_PORT, LED1_PIN);
                    ctx->state = LINK_STATE_IDLE;   /* 复位准备接收下一帧 */
                    /* 通知上层回调（跨核事件桥接等） */
                    if (s_frame_cb != NULL) {
                        s_frame_cb(ctx->addr, ctx->cmd, ctx->data,
                                   ctx->data_len, s_frame_cb_user);
                    }
                    return true;   // 帧完整且CRC正确
                }else {
                    // CRC错误，丢弃帧
                    ctx->state = LINK_STATE_IDLE;
                    return false;
                }
            } else ctx->state = LINK_STATE_IDLE;
            break;

        default:
            ctx->state = LINK_STATE_IDLE;
            break;
    }
    return false;
}

/**
 * @brief 解析缓冲区中的多个字节
 * @note  原实现解析到第一帧即 return，同一缓冲区内后续字节（可能含完整帧）
 *        被直接丢弃，导致多帧同批到达时丢帧。改为解析完所有字节。
 * @return true - 解析到至少一帧完整帧  false - 未解析到完整帧
 */
bool link_parse_buffer(link_parse_ctx_t *ctx, const uint8_t *buffer, uint16_t len)
{
    if (ctx == NULL || buffer == NULL) return false;

    bool got_frame = false;
    for (uint16_t i = 0; i < len; i++) {
        if (link_parse_byte(ctx, buffer[i])) {
            got_frame = true;
        }
    }
    return got_frame;
}







/* 配置CRC */
// void crc_config(void)
// {
//     crc_deinit();
    
//     /* 配置CRC多项式: 32位 */
//     crc_polynomial_size_set(CRC_CTL_PS_32BIT);
    
//     /* 设置CRC多项式 */
//     crc_polynomial_set(0x04C11DB7);
    
//     /* 配置输入数据反转: 按字节 */
//     crc_input_data_reverse_config(CRC_INPUT_DATA_BYTE);
    
//     /* 配置输出数据反转 */
//     crc_reverse_output_data_enable();
// }