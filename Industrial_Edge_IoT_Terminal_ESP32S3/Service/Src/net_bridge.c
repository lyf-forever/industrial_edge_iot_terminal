/**
 * @file net_bridge.c
 * @brief 无线透传桥实现：BLE GATT NUS + LAN TCP Server + mDNS
 *
 * 数据流（双向透传，复用跨核 link_protocol 帧）：
 *   APP --BLE write/TCP--> net_bridge_feed_rx -> link 解析 -> 事件总线/GD32(经 UART)
 *   GD32(经 UART) --link 帧--> event_ipc TX -> net_bridge_broadcast -> APP(BLE notify/TCP)
 */

#include "net_bridge.h"

#if BleGattUse || TcpSrvUse

#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#if EventBusUse && LinkUse
#include "event_ipc.h"
#endif

#if BleGattUse
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#endif

#if TcpSrvUse
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "mdns.h"
#endif

static const char *TAG = "net_bridge";

/* ===================== 无线侧收包 -> 跨核链路 ===================== */

void net_bridge_feed_rx(const uint8_t *data, uint16_t len)
{
#if EventBusUse && LinkUse
    event_ipc_feed_rx(data, len);
#else
    (void)data; (void)len;   /* 无跨核链路时丢弃 */
#endif
}

/* ===================== BLE GATT Server（NUS） ===================== */

#if BleGattUse

/* 128-bit UUID 线上序（小端）：
 * 6E400001-B5A3-F393-E0A9-E50E24DCCA9E → 9E CA DC 24 0E E5 E9 A3 F3 93 B5 A3 01 00 40 6E */
static const uint8_t UUID_SVC[16] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xE9, 0xA3,
    0xF3, 0x93, 0xB5, 0xA3, 0x01, 0x00, 0x40, 0x6E };
static const uint8_t UUID_RX[16] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xE9, 0xA3,
    0xF3, 0x93, 0xB5, 0xA3, 0x02, 0x00, 0x40, 0x6E };
static const uint8_t UUID_TX[16] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xE9, 0xA3,
    0xF3, 0x93, 0xB5, 0xA3, 0x03, 0x00, 0x40, 0x6E };

#define NUS_APP_ID     0x55
#define ADV_NAME       "ind_edge_esp32s3"

static esp_gatt_if_t s_gatts_if = ESP_GATT_IF_NONE;
static uint16_t s_conn_id = 0xFFFF;
static uint16_t s_svc_h = 0;
static uint16_t s_rx_h = 0;
static uint16_t s_tx_h = 0;
static uint16_t s_cccd_h = 0;
static uint16_t s_mtu = 23;
static bool s_connected = false;
static bool s_notify_en = false;
static bool s_adv_data_set = false;
static bool s_scan_rsp_set = false;

static esp_bt_uuid_t uuid128(const uint8_t *b)
{
    esp_bt_uuid_t u = {0};
    u.len = ESP_UUID_LEN_128;
    memcpy(u.uuid.uuid128, b, 16);
    return u;
}

/* ---------- GAP：广播 ---------- */

static void start_advertising(void)
{
    esp_ble_adv_params_t params = {
        .adv_int_min = 0x20,
        .adv_int_max = 0x40,
        .adv_type = ADV_TYPE_IND,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .channel_map = ADV_CHANNEL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };
    esp_ble_gap_start_advertising(&params);
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            s_adv_data_set = true;
            if (s_scan_rsp_set) start_advertising();
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            s_scan_rsp_set = true;
            if (s_adv_data_set) start_advertising();
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "adv start failed: %d", param->adv_start_cmpl.status);
            } else {
                ESP_LOGI(TAG, "BLE advertising as \"%s\"", ADV_NAME);
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            ESP_LOGI(TAG, "BLE adv stopped");
            break;
        default:
            break;
    }
}

static void start_adv_config(void)
{
    esp_bt_uuid_t svc = uuid128(UUID_SVC);
    esp_ble_adv_data_t adv = {
        .set_scan_rsp = false,
        .include_name = false,
        .include_txpower = false,
        .min_interval = 0x20,
        .max_interval = 0x40,
        .appearance = 0x00,
        .service_uuid = &svc,
    };
    esp_ble_gap_config_adv_data(&adv);

    esp_ble_adv_data_t rsp = {
        .set_scan_rsp = true,
        .include_name = true,
    };
    esp_ble_gap_config_adv_data(&rsp);
}

/* ---------- GATTS：服务/特征 ---------- */

static void add_rx_char(void)
{
    esp_bt_uuid_t u = uuid128(UUID_RX);
    esp_ble_gatts_add_char(s_svc_h, &u,
                           ESP_GATT_PERM_WRITE,
                           ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR,
                           NULL, NULL);
}

static void add_tx_char(void)
{
    esp_bt_uuid_t u = uuid128(UUID_TX);
    esp_ble_gatts_add_char(s_svc_h, &u,
                           ESP_GATT_PERM_READ,
                           ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                           NULL, NULL);
}

static void add_cccd_descr(void)
{
    esp_bt_uuid_t u = { .len = ESP_UUID_LEN_16, .uuid = { .uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG } };
    esp_ble_gatts_add_char_descr(s_svc_h, &u,
                                 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                 NULL, NULL);
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            if (param->reg.status != ESP_GATT_OK) {
                ESP_LOGE(TAG, "gatts reg failed: %d", param->reg.status);
                break;
            }
            s_gatts_if = gatts_if;
            esp_ble_gap_set_device_name(ADV_NAME);
            start_adv_config();
            esp_gatt_srvc_id_t svc_id = {
                .is_primary = true,
                .id = { .inst_id = 0, .uuid = uuid128(UUID_SVC) },
            };
            esp_ble_gatts_create_service(gatts_if, &svc_id, 8);
            break;
        }
        case ESP_GATTS_CREATE_EVT:
            s_svc_h = param->create.service_handle;
            ESP_LOGI(TAG, "NUS service created, handle=%d", s_svc_h);
            add_rx_char();
            break;
        case ESP_GATTS_ADD_CHAR_EVT: {
            const esp_bt_uuid_t *u = &param->add_char.uuid;
            if (u->len == ESP_UUID_LEN_128 && memcmp(u->uuid.uuid128, UUID_RX, 16) == 0) {
                s_rx_h = param->add_char.attrib_handle;
                add_tx_char();
            } else if (u->len == ESP_UUID_LEN_128 && memcmp(u->uuid.uuid128, UUID_TX, 16) == 0) {
                s_tx_h = param->add_char.attrib_handle;
                add_cccd_descr();
            }
            break;
        }
        case ESP_GATTS_ADD_CHAR_DESCR_EVT:
            s_cccd_h = param->add_char_descr.attrib_handle;
            esp_ble_gatts_start_service(s_svc_h);
            ESP_LOGI(TAG, "NUS ready (rx=%d tx=%d cccd=%d)", s_rx_h, s_tx_h, s_cccd_h);
            break;
        case ESP_GATTS_CONNECT_EVT:
            s_conn_id = param->connect.conn_id;
            s_connected = true;
            s_notify_en = false;
            ESP_LOGI(TAG, "BLE client connected, conn_id=%d", s_conn_id);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            s_connected = false;
            s_notify_en = false;
            ESP_LOGI(TAG, "BLE client disconnected, restart adv");
            start_advertising();
            break;
        case ESP_GATTS_MTU_EVT:
            s_mtu = param->mtu.mtu;
            ESP_LOGI(TAG, "BLE MTU negotiated: %d", s_mtu);
            break;
        case ESP_GATTS_WRITE_EVT:
            if (param->write.handle == s_rx_h) {
                net_bridge_feed_rx(param->write.value, param->write.len);
            } else if (param->write.handle == s_cccd_h && param->write.len == 2) {
                s_notify_en = (param->write.value[0] & 0x01) != 0;
                ESP_LOGI(TAG, "BLE notify %s", s_notify_en ? "enabled" : "disabled");
            }
            if (param->write.need_rsp) {
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                            param->write.trans_id, ESP_GATT_OK, NULL);
            }
            break;
        default:
            break;
    }
}

static bool ble_notify(const uint8_t *data, uint16_t len)
{
    if (!s_connected || !s_notify_en || s_tx_h == 0) return false;
    uint16_t mtu_payload = (s_mtu > 3) ? (uint16_t)(s_mtu - 3) : 20;
    uint16_t off = 0;
    while (off < len) {
        uint16_t chunk = (uint16_t)((len - off) > mtu_payload ? mtu_payload : (len - off));
        esp_ble_gatts_send_indicate(s_gatts_if, s_conn_id, s_tx_h,
                                    chunk, (uint8_t *)(data + off), false);
        off += chunk;
    }
    return true;
}

static void ble_start(void)
{
    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if (esp_bt_controller_init(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "bt controller init failed");
        return;
    }
    if (esp_bt_controller_enable(ESP_BT_MODE_BLE) != ESP_OK) {
        ESP_LOGE(TAG, "bt controller enable failed");
        return;
    }
    if (esp_bluedroid_init() != ESP_OK || esp_bluedroid_enable() != ESP_OK) {
        ESP_LOGE(TAG, "bluedroid init/enable failed");
        return;
    }
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_app_register(NUS_APP_ID);
    ESP_LOGI(TAG, "BLE GATT start requested (NUS)");
}

#endif /* BleGattUse */

/* ===================== LAN TCP Server + mDNS ===================== */

#if TcpSrvUse

#define TCP_PORT        8080
#define TCP_MAX_CLIENTS 4
#define TCP_RX_BUF      512

static int s_cli[TCP_MAX_CLIENTS];
static SemaphoreHandle_t s_cli_mutex = NULL;

static void tcp_client_clear(int fd)
{
    if (s_cli_mutex) xSemaphoreTake(s_cli_mutex, portMAX_DELAY);
    for (int i = 0; i < TCP_MAX_CLIENTS; i++) {
        if (s_cli[i] == fd) s_cli[i] = -1;
    }
    if (s_cli_mutex) xSemaphoreGive(s_cli_mutex);
}

static void tcp_client_task(void *arg)
{
    int fd = (int)(intptr_t)arg;
    uint8_t buf[TCP_RX_BUF];
    ESP_LOGI(TAG, "TCP client fd=%d connected", fd);
    while (1) {
        int n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) break;
        net_bridge_feed_rx(buf, (uint16_t)n);
    }
    close(fd);
    tcp_client_clear(fd);
    ESP_LOGI(TAG, "TCP client fd=%d disconnected", fd);
    vTaskDelete(NULL);
}

static void tcp_add_client(int fd)
{
    if (s_cli_mutex) xSemaphoreTake(s_cli_mutex, portMAX_DELAY);
    int slot = -1;
    for (int i = 0; i < TCP_MAX_CLIENTS; i++) {
        if (s_cli[i] < 0) { slot = i; break; }
    }
    if (slot < 0) {
        /* 客户端满：踢掉最早的一个 */
        slot = 0;
        close(s_cli[0]);
    }
    s_cli[slot] = fd;
    if (s_cli_mutex) xSemaphoreGive(s_cli_mutex);
    xTaskCreate(tcp_client_task, "tcp_cli", 4096, (void *)(intptr_t)fd, 5, NULL);
}

static void tcp_server_task(void *arg)
{
    (void)arg;
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) {
        ESP_LOGE(TAG, "tcp socket create failed");
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(TCP_PORT),
    };
    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) != 0 ||
        listen(srv, TCP_MAX_CLIENTS) != 0) {
        ESP_LOGE(TAG, "tcp bind/listen failed");
        close(srv);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "TCP passthrough server on port %d", TCP_PORT);

    while (1) {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);
        int cs = accept(srv, (struct sockaddr *)&caddr, &clen);
        if (cs < 0) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }
        tcp_add_client(cs);
    }
}

static void tcp_broadcast(const uint8_t *data, uint16_t len)
{
    if (!s_cli_mutex) return;
    xSemaphoreTake(s_cli_mutex, portMAX_DELAY);
    for (int i = 0; i < TCP_MAX_CLIENTS; i++) {
        if (s_cli[i] >= 0) {
            int r = send(s_cli[i], data, len, MSG_DONTWAIT);
            if (r < 0) {
                close(s_cli[i]);
                s_cli[i] = -1;
            }
        }
    }
    xSemaphoreGive(s_cli_mutex);
}

static void mdns_start(void)
{
    if (mdns_init() != ESP_OK) {
        ESP_LOGW(TAG, "mdns init failed (skip)");
        return;
    }
    mdns_hostname_set("ind-edge");
    mdns_instance_name_set("Industrial Edge IoT Terminal");
    mdns_service_add(NULL, "_ind_edge", "_tcp", TCP_PORT, NULL, 0);
    ESP_LOGI(TAG, "mDNS: ind-edge.local / _ind_edge._tcp:%d", TCP_PORT);
}

static void tcp_start(void)
{
    for (int i = 0; i < TCP_MAX_CLIENTS; i++) s_cli[i] = -1;
    s_cli_mutex = xSemaphoreCreateMutex();
    xTaskCreate(tcp_server_task, "tcp_srv", 4096, NULL, 5, NULL);
    mdns_start();
}

#endif /* TcpSrvUse */

/* ===================== 对外广播 ===================== */

void net_bridge_broadcast(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) return;
#if BleGattUse
    ble_notify(data, len);
#endif
#if TcpSrvUse
    tcp_broadcast(data, len);
#endif
}

/* ===================== 初始化 ===================== */

void net_bridge_init(void *arg)
{
    (void)arg;
#if BleGattUse
    ble_start();
#endif
#if TcpSrvUse
    tcp_start();
#endif
    ESP_LOGI(TAG, "net bridge init done (ble=%d tcp=%d)", BleGattUse, TcpSrvUse);
}

#endif /* BleGattUse || TcpSrvUse */
