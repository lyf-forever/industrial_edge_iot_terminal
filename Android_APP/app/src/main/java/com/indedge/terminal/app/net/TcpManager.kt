package com.indedge.terminal.app.net

import android.os.Handler
import android.os.Looper
import java.io.IOException
import java.net.InetSocketAddress
import java.net.Socket
import kotlin.math.min

/**
 * LAN TCP 透传链路。
 * 对应设计指南中 ESP32-S3 的 Wi-Fi 透传通道（TCP Server 固件规划中，
 * 默认端口 8080，可在“设置”页修改目标地址）。
 *
 * 断线自愈：意外断开后指数退避自动重连（2s/4s/8s/…/30s 封顶，成功清零），
 * 用户主动 disconnect() 后停止重连。
 */
object TcpProfile {
    const val DEFAULT_HOST = "192.168.4.1"
    const val DEFAULT_PORT = 8080
}

interface TcpListener {
    fun onTcpState(state: String, detail: String)
    fun onTcpData(bytes: ByteArray)
}

object TcpManager {

    private val mainHandler = Handler(Looper.getMainLooper())
    private val listeners = mutableSetOf<TcpListener>()

    private var socket: Socket? = null
    private var rxThread: Thread? = null

    private var lastHost = ""
    private var lastPort = 0
    private var userStopped = true
    private var reconnectAttempts = 0

    @Volatile
    var connected = false
        private set

    @Volatile
    var connecting = false
        private set

    private val reconnectRunnable = Runnable { attemptReconnect() }

    fun register(l: TcpListener) = listeners.add(l)

    fun unregister(l: TcpListener) = listeners.remove(l)

    private fun notifyState(state: String, detail: String) {
        mainHandler.post { listeners.forEach { it.onTcpState(state, detail) } }
    }

    /** 用户主动连接：记录目标并进入保活态；若已连接则先切换目标 */
    fun connect(host: String, port: Int) {
        lastHost = host
        lastPort = port
        userStopped = false
        reconnectAttempts = 0
        cancelReconnect()
        if (connected || connecting) {
            // 视为切换目标：先静默停旧链路
            try {
                rxThread?.interrupt()
                socket?.close()
            } catch (_: Exception) {
            }
            socket = null
            connected = false
            connecting = false
        }
        connectInternal()
    }

    private fun connectInternal() {
        if (connecting || connected) return
        if (lastHost.isEmpty()) return
        connecting = true
        notifyState("CONNECTING", "$lastHost:$lastPort")
        Thread {
            try {
                val s = Socket()
                s.connect(InetSocketAddress(lastHost, lastPort), 5000)
                socket = s
                connecting = false
                connected = true
                reconnectAttempts = 0
                cancelReconnect()
                notifyState("CONNECTED", "$lastHost:$lastPort 已连接")
                startRxLoop()
            } catch (e: IOException) {
                connecting = false
                connected = false
                if (userStopped) {
                    notifyState("ERROR", "TCP 连接失败: ${e.message}")
                } else {
                    notifyState("RECONNECTING", "TCP 连接失败: ${e.message}，自动重连中")
                    scheduleReconnect()
                }
            }
        }.start()
    }

    private fun startRxLoop() {
        val s = socket ?: return
        rxThread = Thread {
            val buf = ByteArray(1024)
            try {
                val ins = s.getInputStream()
                while (!Thread.currentThread().isInterrupted) {
                    val n = ins.read(buf)
                    if (n > 0) {
                        val chunk = buf.copyOf(n)
                        mainHandler.post { listeners.forEach { it.onTcpData(chunk) } }
                    }
                }
            } catch (_: IOException) {
                // 读取失败即链路断开
            } finally {
                // 仅当仍是当前链路的接收循环时才更新状态（避免切换目标时覆盖新连接）
                if (socket === s) {
                    connected = false
                    if (userStopped) {
                        notifyState("DISCONNECTED", "TCP 连接已断开")
                    } else {
                        notifyState("RECONNECTING", "TCP 连接断开，自动重连中")
                        scheduleReconnect()
                    }
                }
            }
        }.also { it.start() }
    }

    /** 指数退避：2s/4s/8s/16s/30s 封顶 */
    private fun scheduleReconnect() {
        if (userStopped) return
        val delayMs = min(2_000L shl reconnectAttempts, 30_000L)
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

    fun send(bytes: ByteArray): Boolean {
        return try {
            socket?.getOutputStream()?.write(bytes)
            socket?.getOutputStream()?.flush()
            true
        } catch (e: IOException) {
            notifyState("ERROR", "发送失败: ${e.message}")
            false
        }
    }

    /** 用户主动断开：停止自动重连 */
    fun disconnect() {
        val wasUp = connected || connecting
        userStopped = true
        cancelReconnect()
        connecting = false
        try {
            rxThread?.interrupt()
            socket?.close()
        } catch (_: Exception) {
        }
        socket = null
        connected = false
        if (wasUp) {
            notifyState("DISCONNECTED", "TCP 连接已断开")
        }
    }
}
