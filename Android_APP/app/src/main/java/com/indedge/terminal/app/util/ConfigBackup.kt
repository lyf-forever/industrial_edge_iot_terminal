package com.indedge.terminal.app.util

import android.content.Context
import org.json.JSONObject

/**
 * 全量配置备份/恢复（JSON 文件）。
 * 覆盖 MQTT / BLE 契约 UUID / TCP / 保活开关等全部持久化参数。
 */
object ConfigBackup {

    private const val VERSION = 1

    /** 字符串键清单 */
    private val strKeys = listOf(
        Prefs.KEY_MQTT_URI,
        Prefs.KEY_CLIENT_ID,
        Prefs.KEY_BLE_SERVICE_UUID,
        Prefs.KEY_BLE_TX_UUID,
        Prefs.KEY_BLE_RX_UUID,
        Prefs.KEY_TCP_HOST,
        Prefs.KEY_KEEP_ALIVE
    )

    fun exportJson(context: Context): String {
        val o = JSONObject()
        o.put("version", VERSION)
        for (k in strKeys) {
            o.put(k, Prefs.str(context, k, ""))
        }
        o.put(Prefs.KEY_TCP_PORT, Prefs.int(context, Prefs.KEY_TCP_PORT, 8080))
        return o.toString(2)
    }

    /**
     * 导入 JSON 配置；键白名单校验，仅应用已知字段。
     * @return 是否成功
     */
    fun importJson(context: Context, json: String): Boolean {
        return try {
            val o = JSONObject(json)
            if (o.optInt("version") != VERSION) return false
            var applied = false
            for (k in strKeys) {
                if (o.has(k)) {
                    Prefs.setStr(context, k, o.getString(k))
                    applied = true
                }
            }
            if (o.has(Prefs.KEY_TCP_PORT)) {
                Prefs.setInt(context, Prefs.KEY_TCP_PORT, o.getInt(Prefs.KEY_TCP_PORT))
                applied = true
            }
            applied
        } catch (_: Exception) {
            false
        }
    }
}
