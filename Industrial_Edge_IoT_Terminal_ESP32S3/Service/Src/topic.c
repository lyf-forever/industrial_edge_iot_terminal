/**
 * @file topic.c
 * @brief 主题化发布订阅实现：事件<->主题双向映射 + 订阅表 + 通配匹配
 */

#include "topic.h"

#if TopicUse && EventBusUse

#include "event_bus.h"
#include "esp_log.h"
#include <string.h>

#define TOPIC_SUB_MAX   8
#define TOPIC_MAP_MAX   16

typedef struct {
    bool    used;
    char    pattern[TOPIC_MAX_LEN];
    void  (*cb)(uint16_t, const uint8_t *, uint8_t, uint8_t, void *);
    void   *user;
} topic_sub_t;

/* 事件 ID <-> 主题双向映射表 */
typedef struct {
    bool    used;
    uint16_t event_id;
    char     topic[TOPIC_MAX_LEN];
} topic_map_t;

static topic_sub_t s_subs[TOPIC_SUB_MAX];
static topic_map_t s_maps[TOPIC_MAP_MAX];

/* 内置域级映射（sensor/comm/ctrl/sys） */
static const struct { uint16_t domain; const char *name; } s_domain_map[] = {
    { 0x0100, "sensor" },
    { 0x0200, "comm"   },
    { 0x0300, "ctrl"   },
    { 0x0400, "sys"    },
};

/* ===================== 双向映射 ===================== */

/* 注册事件 ID <-> 主题映射（如 EVT_SENSOR_DATA -> "sensor.mq2"） */
bool topic_register_map(uint16_t event_id, const char *topic)
{
    if (topic == NULL || strlen(topic) >= TOPIC_MAX_LEN) return false;
    for (int i = 0; i < TOPIC_MAP_MAX; i++) {
        if (!s_maps[i].used) {
            s_maps[i].used = true;
            s_maps[i].event_id = event_id;
            strncpy(s_maps[i].topic, topic, TOPIC_MAX_LEN - 1);
            s_maps[i].topic[TOPIC_MAX_LEN - 1] = '\0';
            return true;
        }
        if (s_maps[i].event_id == event_id) {   /* 已存在：更新 */
            strncpy(s_maps[i].topic, topic, TOPIC_MAX_LEN - 1);
            s_maps[i].topic[TOPIC_MAX_LEN - 1] = '\0';
            return true;
        }
    }
    return false;
}

/* 事件 ID -> 主题（查映射表；未注册则回退域级名） */
const char *topic_of_event(uint16_t event_id)
{
    for (int i = 0; i < TOPIC_MAP_MAX; i++) {
        if (s_maps[i].used && s_maps[i].event_id == event_id) {
            return s_maps[i].topic;
        }
    }
    uint16_t domain = event_id & 0xFF00;
    for (size_t i = 0; i < sizeof(s_domain_map) / sizeof(s_domain_map[0]); i++) {
        if (s_domain_map[i].domain == domain) {
            return s_domain_map[i].name;
        }
    }
    return "unknown";
}

/* 主题 -> 事件 ID（精确匹配映射表；-1 未注册） */
int16_t topic_event_of(const char *topic)
{
    if (topic == NULL) return -1;
    for (int i = 0; i < TOPIC_MAP_MAX; i++) {
        if (s_maps[i].used && strcmp(s_maps[i].topic, topic) == 0) {
            return (int16_t)s_maps[i].event_id;
        }
    }
    return -1;
}

/* ===================== 匹配与订阅 ===================== */

bool topic_match(const char *pattern, const char *topic)
{
    if (pattern == NULL || topic == NULL) return false;
    if (strcmp(pattern, TOPIC_WILDCARD) == 0) return true;

    size_t plen = strlen(pattern);
    if (plen >= 2 && strcmp(pattern + plen - 2, ".*") == 0) {
        size_t prefix = plen - 2;
        return strncmp(pattern, topic, prefix) == 0;
    }
    return strcmp(pattern, topic) == 0;
}

int8_t topic_subscribe(const char *topic,
                       void (*cb)(uint16_t, const uint8_t *, uint8_t, uint8_t, void *),
                       void *user)
{
    if (topic == NULL || cb == NULL) return -1;
    if (strlen(topic) >= TOPIC_MAX_LEN) return -1;
    for (int i = 0; i < TOPIC_SUB_MAX; i++) {
        if (!s_subs[i].used) {
            s_subs[i].used = true;
            strncpy(s_subs[i].pattern, topic, TOPIC_MAX_LEN - 1);
            s_subs[i].pattern[TOPIC_MAX_LEN - 1] = '\0';
            s_subs[i].cb = cb;
            s_subs[i].user = user;
            return (int8_t)i;
        }
    }
    return -1;
}

void topic_unsubscribe(int8_t sub)
{
    if (sub < 0 || sub >= TOPIC_SUB_MAX) return;
    s_subs[sub].used = false;
}

/* 由调用方在收到事件后调用：将事件向主题订阅者转发 */
bool topic_dispatch(uint16_t event_id, const uint8_t *payload, uint8_t len,
                    uint8_t source, void *user)
{
    (void)user;
    const char *evt_topic = topic_of_event(event_id);
    bool matched = false;
    for (int i = 0; i < TOPIC_SUB_MAX; i++) {
        if (!s_subs[i].used) continue;
        if (topic_match(s_subs[i].pattern, evt_topic)) {
            if (s_subs[i].cb) {
                s_subs[i].cb(event_id, payload, len, source, s_subs[i].user);
                matched = true;
            }
        }
    }
    return matched;
}

#endif /* TopicUse && EventBusUse */