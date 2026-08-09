#include "link_payload.h"
#include <string.h>

void link_payload_sensor_pack(const sensor_payload_t *payload, uint8_t *data)
{
    if (payload == NULL || data == NULL) return;
    memcpy(data, payload, LINK_MAX_DATA_LEN);
}

void link_payload_sensor_unpack(const uint8_t *data, sensor_payload_t *payload)
{
    if (payload == NULL || data == NULL) return;
    memcpy(payload, data, LINK_MAX_DATA_LEN);
}

void link_payload_alarm_pack(const alarm_payload_t *payload, uint8_t *data)
{
    if (payload == NULL || data == NULL) return;
    memcpy(data, payload, LINK_MAX_DATA_LEN);
}

void link_payload_alarm_unpack(const uint8_t *data, alarm_payload_t *payload)
{
    if (payload == NULL || data == NULL) return;
    memcpy(payload, data, LINK_MAX_DATA_LEN);
}

void link_payload_led_pack(const led_ctrl_payload_t *payload, uint8_t *data)
{
    if (payload == NULL || data == NULL) return;
    memcpy(data, payload, LINK_MAX_DATA_LEN);
}

void link_payload_led_unpack(const uint8_t *data, led_ctrl_payload_t *payload)
{
    if (payload == NULL || data == NULL) return;
    memcpy(payload, data, LINK_MAX_DATA_LEN);
}

void link_payload_event_pack(const event_payload_t *payload, uint8_t *data)
{
    if (payload == NULL || data == NULL) return;
    memcpy(data, payload, LINK_MAX_DATA_LEN);
}

void link_payload_event_unpack(const uint8_t *data, event_payload_t *payload)
{
    if (payload == NULL || data == NULL) return;
    memcpy(payload, data, LINK_MAX_DATA_LEN);
}