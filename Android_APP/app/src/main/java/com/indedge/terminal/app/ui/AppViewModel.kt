package com.indedge.terminal.app.ui

import android.app.Application
import android.os.Handler
import android.os.Looper
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.MutableLiveData
import com.indedge.terminal.app.db.SensorRecord
import com.indedge.terminal.app.db.SensorStore
import com.indedge.terminal.app.mqtt.MqttEventListener
import com.indedge.terminal.app.mqtt.MqttManager
import com.indedge.terminal.app.mqtt.MqttProtocol
import com.indedge.terminal.app.util.AlarmPrefs
import com.indedge.terminal.app.util.Notifier
import com.indedge.terminal.app.util.TimeFmt
import org.json.JSONObject
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * 活动级共享 ViewModel（整个 APP 会话存活）：
 * 作为 MqttManager 的唯一业务订阅者，集中持有监控数据与连接状态，
 * 使历史曲线 / 日志 / 告警 / 命令历史 / 在线状态在页面切换后不丢失。
 *
 * 继承 AndroidViewModel 以便持有 Application：
 * 高等级告警通知不依赖任何页面处于前台，直接推送。
 */
class AppViewModel(application: Application) : AndroidViewModel(application), MqttEventListener {

    data class ConnState(val connected: Boolean, val detail: String)
    data class SensorData(val temp: Float, val humid: Float, val gas: Int, val co2: Int, val press: Int)
    data class PubEvent(val topic: String, val ok: Boolean)
    data class AlarmEvent(val id: Int, val level: Int, val value: Int, val line: String)

    companion object {
        const val SERIES_COUNT = 5
        const val SERIES_MAX = 60
        const val LOG_MAX = 200
        const val ALARM_MAX = 50
        const val CMD_MAX = 50
        const val HB_TIMEOUT_MS = 60_000L
        const val DB_THROTTLE_MS = 3_000L
    }

    // ---- 连接 ----
    val connection = MutableLiveData(ConnState(false, "未连接"))
    val publishResult = MutableLiveData<PubEvent?>(null)

    // ---- 传感器 ----
    val sensorLatest = MutableLiveData<SensorData?>(null)
    val sensorSeries = Array(SERIES_COUNT) { ArrayDeque<Float>() }

    // ---- 日志 ----
    val logEvent = MutableLiveData<String?>(null)
    val logLines = ArrayDeque<String>()

    // ---- 告警（列表 newest-first） ----
    val alarmEvent = MutableLiveData<AlarmEvent?>(null)
    val alarmLines = ArrayDeque<String>()

    // ---- 心跳 / 在线 ----
    val online = MutableLiveData(false)
    val hbLine = MutableLiveData("暂无心跳 · 运行 —")

    // ---- 命令历史（oldest-first） ----
    val cmdEvent = MutableLiveData<String?>(null)
    val cmdLines = ArrayDeque<String>()

    private var lastHeartbeatAt = 0L
    private var lastUptime = 0L
    private var lastDbInsertAt = 0L

    private val timeFmt = SimpleDateFormat("HH:mm:ss", Locale.getDefault())
    private val mainHandler = Handler(Looper.getMainLooper())

    private val ticker = object : Runnable {
        override fun run() {
            updateOnline()
            mainHandler.postDelayed(this, 2000)
        }
    }

    init {
        MqttManager.register(this)
        updateOnline()
        mainHandler.postDelayed(ticker, 2000)
    }

    override fun onCleared() {
        mainHandler.removeCallbacks(ticker)
        MqttManager.unregister(this)
    }

    // ================= MqttEventListener =================

    override fun onConnectionChanged(connected: Boolean, detail: String) {
        connection.value = ConnState(connected, detail)
        updateOnline()
    }

    override fun onMessage(topic: String, payload: String) {
        appendLog("[${topic.substringAfterLast('/')}] $payload")
        when (topic) {
            MqttProtocol.TOPIC_SENSORS -> onSensors(payload)
            MqttProtocol.TOPIC_ALARM -> onAlarm(payload)
            MqttProtocol.TOPIC_STATUS -> onStatus(payload)
        }
    }

    override fun onPublishResult(topic: String, ok: Boolean) {
        publishResult.value = PubEvent(topic, ok)
        if (topic == MqttProtocol.TOPIC_CMD) {
            appendCmd(if (ok) "  已确认" else "  发送失败")
        } else {
            // 调试面板等自定义主题的发布结果回显到日志
            appendLog(if (ok) "发布成功: $topic" else "发布失败: $topic")
        }
    }

    private fun onSensors(payload: String) {
        try {
            val o = JSONObject(payload)
            val temp = o.optDouble("temp").toFloat()
            val humid = o.optDouble("humid").toFloat()
            val gas = o.optInt("gas")
            val co2 = o.optInt("co2")
            val press = o.optInt("press")

            sensorLatest.value = SensorData(temp, humid, gas, co2, press)

            val values = arrayOf(temp, humid, gas.toFloat(), co2.toFloat(), press.toFloat())
            for (i in values.indices) {
                val q = sensorSeries[i]
                q.addLast(values[i])
                while (q.size > SERIES_MAX) q.removeFirst()
            }

            // 历史落盘节流
            val now = System.currentTimeMillis()
            if (now - lastDbInsertAt >= DB_THROTTLE_MS) {
                lastDbInsertAt = now
                SensorStore.insert(SensorRecord(now, temp, humid, gas, co2, press))
            }
        } catch (_: Exception) {
            appendLog("传感器报文解析失败: $payload")
        }
    }

    private fun onAlarm(payload: String) {
        try {
            val o = JSONObject(payload)
            val id = o.optInt("alarm_id")
            val level = o.optInt("level")
            val value = o.optInt("val")
            val line = "[${timeFmt.format(Date())}] 告警 #$id 等级 $level 值 $value"
            alarmLines.addFirst(line)
            while (alarmLines.size > ALARM_MAX) alarmLines.removeLast()
            alarmEvent.value = AlarmEvent(id, level, value, line)
            // 高等级告警通知不依赖页面前台状态；受通知总开关与勿扰时段门控
            if (level >= 2 && AlarmPrefs.shouldNotify(getApplication())) {
                Notifier.notifyAlarm(getApplication(), id, level, value)
            }
        } catch (_: Exception) {
            appendLog("告警报文解析失败: $payload")
        }
    }

    private fun onStatus(payload: String) {
        try {
            val o = JSONObject(payload)
            lastHeartbeatAt = System.currentTimeMillis()
            lastUptime = o.optLong("uptime_s")
            updateOnline()
        } catch (_: Exception) {
            appendLog("状态报文解析失败: $payload")
        }
    }

    // ================= 状态计算与数据接口 =================

    private fun updateOnline() {
        val ok = MqttManager.connected &&
            lastHeartbeatAt > 0 &&
            (System.currentTimeMillis() - lastHeartbeatAt) < HB_TIMEOUT_MS
        online.value = ok
        hbLine.value = if (lastHeartbeatAt == 0L) {
            "暂无心跳 · 运行 —"
        } else {
            "最后心跳 ${timeFmt.format(Date(lastHeartbeatAt))} · 运行 ${TimeFmt.formatUptime(lastUptime)}"
        }
    }

    fun appendLog(line: String) {
        logLines.addLast(line)
        while (logLines.size > LOG_MAX) logLines.removeFirst()
        logEvent.value = line
    }

    fun appendCmd(line: String) {
        cmdLines.addLast(line)
        while (cmdLines.size > CMD_MAX) cmdLines.removeFirst()
        cmdEvent.value = line
    }

    /** 供图表重建：按序列索引取历史点 */
    fun seedSensorSeries(index: Int): List<Float> = sensorSeries[index].toList()

    fun logText(): String = logLines.joinToString("\n")

    fun cmdText(): String = cmdLines.joinToString("\n")

    fun clearLogs() {
        logLines.clear()
        logEvent.value = null
    }

    fun clearAlarms() {
        alarmLines.clear()
        alarmEvent.value = null
    }

    fun clearCmds() {
        cmdLines.clear()
        cmdEvent.value = null
    }

    fun clearSeries() {
        for (q in sensorSeries) q.clear()
    }
}
