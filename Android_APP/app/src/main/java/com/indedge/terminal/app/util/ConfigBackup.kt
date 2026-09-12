package com.indedge.terminal.app.util

import android.content.Context
import org.json.JSONObject

/**
 * 全量配置备份/恢复（JSON 文件）。
 * 覆盖 MQTT / BLE 契约 UUID / TCP / 保活开关等全部持久化参数。
 *
 * 核心为纯函数（buildJson/parse），便于 JVM 单测；
 * exportJson/importJson 为 Context 包装。
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

    /** 纯函数：按键清单构建 JSON（缺失键以空串填充） */
    fun buildJson(values: Map<String, String>, tcpPort: Int): String {
        val o = JSONObject()
        o.put("version", VERSION)
        for (k in strKeys) {
            o.put(k, values[k] ?: "")
        }
        o.put(Prefs.KEY_TCP_PORT, tcpPort)
        return o.toString(2)
    }

    /**
     * 纯函数：解析并校验 JSON（版本号 + 键白名单）。
     * @return 键->值映射（端口为字符串形式）；版本不符/格式错误返回 null
     */
    fun parse(json: String): Map<String, String>? {
        return try {
            val o = JSONObject(json)
            if (o.optInt("version") != VERSION) return null
            val out = mutableMapOf<String, String>()
            for (k in strKeys) {
                if (o.has(k)) out[k] = o.getString(k)
            }
            if (o.has(Prefs.KEY_TCP_PORT)) {
                out[Prefs.KEY_TCP_PORT] = o.getInt(Prefs.KEY_TCP_PORT).toString()
            }
            if (out.isEmpty()) null else out
        } catch (_: Exception) {
            null
        }
    }

    fun exportJson(context: Context): String =
        buildJson(
            strKeys.associateWith { Prefs.str(context, it, "") },
            Prefs.int(context, Prefs.KEY_TCP_PORT, 8080)
        )

    /**
     * 导入 JSON 配置；键白名单校验，仅应用已知字段。
     * @return 是否成功
     */
    fun importJson(context: Context, json: String): Boolean {
        val map = parse(json) ?: return false
        // 先整体校验端口，避免部分应用
        var port: Int? = null
        if (map.containsKey(Prefs.KEY_TCP_PORT)) {
            port = map[Prefs.KEY_TCP_PORT]?.toIntOrNull() ?: return false
        }
        for ((k, v) in map) {
            if (k == Prefs.KEY_TCP_PORT) continue
            Prefs.setStr(context, k, v)
        }
        port?.let { Prefs.setInt(context, Prefs.KEY_TCP_PORT, it) }
        return true
    }
}
