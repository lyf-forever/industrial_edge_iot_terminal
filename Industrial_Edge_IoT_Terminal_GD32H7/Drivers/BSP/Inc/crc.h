#ifndef CRC16_H_
#define CRC16_H_

#include <stdint.h>

uint16_t crc16_modbus(const uint8_t *data, uint16_t len);

#endif