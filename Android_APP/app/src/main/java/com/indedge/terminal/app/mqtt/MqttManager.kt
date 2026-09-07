package com.indedge.terminal.app.mqtt

import android.content.Context
import android.net.ConnectivityManager
import android.net.Network
import android.os.Handler
import android.os.Looper
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended
import org.eclipse.paho.client.mqttv3.MqttClient
import org.eclipse.paho.client.mqttv3.MqttConnectOptions
import org.eclipse.paho.client.mqttv3.MqttException
import org.eclipse.paho.client.mqttv3.MqttMessage
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence
import java.util.UUID
import kotlin.math.min

/**
 * 与固件共享的 MQTT 契约（对照 ESP32 端 Service/Inc/mqtt_client_app.h）：
 *
 * Broker : tcp://broker.emqx.io:1883（可改 OneNET 等平台地址）
 * 上行主题（设备 -> 云）:
 *   ind_edge/terminal/sensors  {"temp":xx.x,"humid":xx.x,"gas":N,"co2":N,"press":N}
 *   ind_edge/terminal/alarm    {"alarm_id":N,"level":N,"val":N}
 *   ind_edge/terminal/status   {"heartbeat":N,"uptime_s":N}
 * 下行主题（云 -> 设备）:
 *   ind_edge/terminal/cmd      {"cmd":"led","id":1,"state":1}
 *                              {"cmd":"status"}
 *                              {"cmd":"reboot"}
 *                              {"cmd":"ota","url":"https://..."}
 */
object MqttProtocol {
    const val DEFAULT_URI = "tcp://broker.emqx.io:1883"
    const val DEFAULT_CLIENT_ID_PREFIX = "ind_edge_app_android_"

    const val TOPIC_SENSORS = "ind_edge/terminal/sensors"
    const val TOPIC_ALARM = "ind_edge/terminal/alarm"
    const val TOPIC_STATUS = "ind_edge/terminal/status"
    const val TOPIC_CMD = "ind_edge/terminal/cmd"

    fun defaultClientId(): String = DEFAULT_CLIENT_ID_PREFIX + UUID.randomUUID()
        .toString().take(8)
}

/** UI 层订阅 MQTT 事件的回调（全部在主线程投递） */
interface MqttEventListener {
    fun onConnectionChanged(connected: Boolean, detail: String)
    fun onMessage(topic: String, payload: String)
    /** QoS 发布确认回调（主线程），默认空实现，按需覆写 */
    fun onPublishResult(topic: String, ok: Boolean) {
        // 默认不处理
    }
}

/**
 * MQTT 连接管理器（进程内单例）。
 * 职责：连接/订阅/命令发布 + 自愈重连。
 *
 * 重连策略（自实现，不依赖 Paho isAutomaticReconnect）：
 *   - 断开后指数退避重连：2s/4s/8s/16s/32s/60s（封顶），成功清零；
 *   - 网络恢复（ConnectivityManager 回调）立即触发一次重试；
 *   - 用户主动 disconnect() 后停止一切自动重连。
 */
object MqttManager {

    private val mainHandler = Handler(Looper.getMainLooper())
    private val listeners = mutableSetOf<MqttEventListener>()

    private var client: MqttClient? = null
    private var appCtx: Context? = null

    private var uri = ""
    private var clientId = ""
    private var reconnectAttempts = 0
    private var userStopped = true

    @Volatile
    var connecting = false
        private set

    @Volatile
    var connected = false
        private set

    private val reconnectRunnable = Runnable { attemptReconnect() }

    private val networkCallback = object : ConnectivityManager.NetworkCallback() {
        override fun onAvailable(network: Network) {
            // 网络恢复立即触发一次重试
            mainHandler.post { attemptReconnect() }
        }
    }

    /** 进程启动时初始化（注册网络监听） */
    fun init(context: Context) {
        if (appCtx != null) return
        appCtx = context.applicationContext
        try {
            val cm = appCtx?.getSystemService(Context.CONNECTIVITY_SERVICE) as? ConnectivityManager
            cm?.registerDefaultNetworkCallback(networkCallback)
        } catch (_: Exception) {
        }
    }

    fun register(l: MqttEventListener) {
        listeners.add(l)
        mainHandler.post { l.onConnectionChanged(connected, if (connected) "已连接" else "未连接") }
    }

    fun unregister(l: MqttEventListener) {
        listeners.remove(l)
    }

    private fun notifyConnection(ok: Boolean, detail: String) {
        connected = ok
        mainHandler.post { listeners.forEach { it.onConnectionChanged(ok, detail) } }
    }

    // ================= 连接 / 重连 =================

    /** 用户主动连接：重置退避，进入保活态 */
    fun connect(newUri: String, newClientId: String) {
        uri = newUri
        clientId = newClientId
        userStopped = false
        reconnectAttempts = 0
        cancelReconnect()
        if (connecting) return
        connectInternal()
    }

    private fun connectInternal() {
        if (connecting) return
        connecting = true
        Thread {
            try {
                client?.disconnectForcibly()
                client?.close()

                val c = MqttClient(uri, clientId, MemoryPersistence())
                val opts = MqttConnectOptions().apply {
                    isCleanSession = true
                    // 重连由本管理器自行调度
                    isAutomaticReconnect = false
                    connectionTimeout = 10
                    keepAliveInterval = 30
                }
                c.setCallback(mqttCallback)
                c.connect(opts)
                client = c
            } catch (e: MqttException) {
                connecting = false
                notifyConnection(false, "连接失败: ${e.message}")
                scheduleReconnect()
            } catch (e: Exception) {
                connecting = false
                notifyConnection(false, "连接失败: ${e.message}")
                scheduleReconnect()
            }
        }.start()
    }

    private val mqttCallback = object : MqttCallbackExtended {

        override fun connectComplete(reconnect: Boolean, serverURI: String) {
            connecting = false
            // 连接成功：清零退避计数并停止待执行的重连
            reconnectAttempts = 0
            cancelReconnect()
            subscribeAll()
            val detail = if (reconnect) "重连成功: $serverURI" else "连接成功: $serverURI"
            notifyConnection(true, detail)
        }

        override fun connectionLost(cause: Throwable?) {
            connecting = false
            notifyConnection(false, "连接断开: ${cause?.message ?: "未知原因"}")
            scheduleReconnect()
        }

        override fun messageArrived(topic: String, message: MqttMessage) {
            val payload = String(message.payload, Charsets.UTF_8)
            mainHandler.post { listeners.forEach { it.onMessage(topic, payload) } }
        }

        override fun deliveryComplete(token: IMqttDeliveryToken?) {
            // 命令送达回调（QoS1 确认经 IMqttActionListener 通知）
        }
    }

    /** 指数退避重连：2s/4s/8s/16s/32s/60s 封顶 */
    private fun scheduleReconnect() {
        if (userStopped) return
        val delayMs = min(2_000L shl reconnectAttempts, 60_000L)
        reconnectAttempts++
        mainHandler.removeCallbacks(reconnectRunnable)
        mainHandler.postDelayed(reconnectRunnable, delayMs)
    }

    private fun attemptReconnect() {
        if (userStopped || connected || connecting) return
        connectInternal()
    }

    private fun cancelReconnect() {
        mainHandler.removeCallbacks(reconnectRunnable)
    }

    /** 用户主动断开：停止自动重连 */
    fun disconnect() {
        userStopped = true
        cancelReconnect()
        connecting = false
        try {
            client?.disconnect()
            client?.close()
        } catch (_: Exception) {
        }
        client = null
        connected = false
    }

    // ================= 订阅 / 发布 =================

    private fun subscribeAll() {
        try {
            client?.subscribe(MqttProtocol.TOPIC_SENSORS, 1)
            client?.subscribe(MqttProtocol.TOPIC_ALARM, 1)
            client?.subscribe(MqttProtocol.TOPIC_STATUS, 1)
        } catch (e: MqttException) {
            notifyConnection(false, "订阅失败: ${e.message}")
        }
    }

    /** 订阅任意主题（调试面板用），QoS 1 */
    fun subscribeRaw(topic: String): Boolean {
        return try {
            client?.subscribe(topic, 1)
            true
        } catch (e: MqttException) {
            notifyConnection(false, "订阅失败: ${e.message}")
            false
        }
    }

    /** 退订任意主题（调试面板用） */
    fun unsubscribeRaw(topic: String): Boolean {
        return try {
            client?.unsubscribe(topic)
            true
        } catch (e: MqttException) {
            false
        }
    }

    /**
     * 发布到任意主题（QoS1）。
     * Paho MqttClient（同步客户端）的 publish 会阻塞至 broker 确认（QoS>0）或超时，
     * 无异常即视为送达，结果经 onPublishResult 通知（主线程）。
     */
    fun publishRaw(topic: String, payload: String, qos: Int = 1): Boolean {
        return try {
            val c = client ?: return false
            val msg = MqttMessage(payload.toByteArray(Charsets.UTF_8)).apply { this.qos = qos }
            c.publish(topic, msg)
            mainHandler.post { listeners.forEach { it.onPublishResult(topic, true) } }
            true
        } catch (e: MqttException) {
            mainHandler.post { listeners.forEach { it.onPublishResult(topic, false) } }
            notifyConnection(false, "发布失败: ${e.message}")
            false
        }
    }

    /** 向 ind_edge/terminal/cmd 发布控制命令（JSON 字符串） */
    fun publishCommand(json: String): Boolean = publishRaw(MqttProtocol.TOPIC_CMD, json, 1)
}
