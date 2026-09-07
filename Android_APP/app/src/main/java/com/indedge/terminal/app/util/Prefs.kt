package com.indedge.terminal.app.util

import android.content.Context

/**
 * 全局配置持久化（SharedPreferences 封装）。
 */
object Prefs {
    private const val NAME = "ind_edge_app"

    const val KEY_MQTT_URI = "mqtt_uri"
    const val KEY_CLIENT_ID = "client_id"

    const val KEY_BLE_SERVICE_UUID = "ble_service_uuid"
    const val KEY_BLE_TX_UUID = "ble_tx_uuid"
    const val KEY_BLE_RX_UUID = "ble_rx_uuid"

    const val KEY_TCP_HOST = "tcp_host"
    const val KEY_TCP_PORT = "tcp_port"

    const val KEY_KEEP_ALIVE = "keep_alive"

    fun str(context: Context, key: String, def: String): String =
        context.getSharedPreferences(NAME, Context.MODE_PRIVATE).getString(key, def) ?: def

    fun setStr(context: Context, key: String, value: String) {
        context.getSharedPreferences(NAME, Context.MODE_PRIVATE).edit().putString(key, value).apply()
    }

    fun int(context: Context, key: String, def: Int): Int =
        context.getSharedPreferences(NAME, Context.MODE_PRIVATE).getInt(key, def)

    fun setInt(context: Context, key: String, value: Int) {
        context.getSharedPreferences(NAME, Context.MODE_PRIVATE).edit().putInt(key, value).apply()
    }
}
