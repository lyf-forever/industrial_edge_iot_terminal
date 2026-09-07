package com.indedge.terminal.app.bt

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import android.os.Handler
import android.os.Looper
import java.io.IOException
import kotlin.math.min

/**
 * 经典蓝牙 SPP 透传链路。
 * 对应 GD32H7 端 BT24 / HC05 蓝牙模块（9600bps 串口透传），
 * 以及 ESP32-S3 内置经典蓝牙 SPP（设计指南 Phase 2）。
 *
 * 断线自愈：意外断开后指数退避自动重连（2s/4s/8s/…/30s 封顶，成功清零），
 * 用户主动 disconnect() 后停止重连。
 */
object SppProfile {
    /** 标准 SPP 服务 UUID */
    val UUID_SPP: java.util.UUID = java.util.UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")
}

interface SppListener {
    fun onSppState(state: String, detail: String)
    fun onSppData(bytes: ByteArray)
}

@SuppressLint("MissingPermission")
object SppManager {

    private val mainHandler = Handler(Looper.getMainLooper())
    private val listeners = mutableSetOf<SppListener>()

    private var adapter: BluetoothAdapter? = null
    private var socket: BluetoothSocket? = null
    private var rxThread: Thread? = null

    private var lastDevice: BluetoothDevice? = null
    private var userStopped = true
    private var reconnectAttempts = 0

    @Volatile
    var connected = false
        private set

    @Volatile
    var connecting = false
        private set

    /** 当前已连接设备（供 UI 列表高亮） */
    @Volatile
    var connectedDevice: BluetoothDevice? = null
        private set

    private val reconnectRunnable = Runnable { attemptReconnect() }

    fun btAdapter(): BluetoothAdapter? {
        adapter = adapter ?: BluetoothAdapter.getDefaultAdapter()
        return adapter
    }

    fun isBluetoothEnabled(): Boolean = btAdapter()?.isEnabled == true

    fun register(l: SppListener) = listeners.add(l)

    fun unregister(l: SppListener) = listeners.remove(l)

    private fun notifyState(state: String, detail: String) {
        mainHandler.post { listeners.forEach { it.onSppState(state, detail) } }
    }

    @SuppressLint("MissingPermission")
    fun pairedDevices(): List<BluetoothDevice> =
        btAdapter()?.bondedDevices?.toList() ?: emptyList()

    /** 用户主动连接：记录目标设备并进入保活态；若已连接其他设备则先切换目标 */
    @SuppressLint("MissingPermission")
    fun connect(device: BluetoothDevice) {
        lastDevice = device
        userStopped = false
        reconnectAttempts = 0
        cancelReconnect()
        if (connected || connecting) {
            // 正在连接/已连接：视为切换目标，先静默停旧链路
            try {
                rxThread?.interrupt()
                socket?.close()
            } catch (_: Exception) {
            }
            socket = null
            connected = false
            connecting = false
            connectedDevice = null
        }
        connectInternal()
    }

    @SuppressLint("MissingPermission")
    private fun connectInternal() {
        if (connecting || connected) return
        val d = lastDevice ?: return
        connecting = true
        notifyState("CONNECTING", d.address)
        Thread {
            try {
                val s = d.createRfcommSocketToServiceRecord(SppProfile.UUID_SPP)
                s.connect()
                socket = s
                connecting = false
                connected = true
                connectedDevice = d
                reconnectAttempts = 0
                cancelReconnect()
                notifyState("CONNECTED", "${d.name ?: d.address} 已连接")
                startRxLoop()
            } catch (e: IOException) {
                connecting = false
                connected = false
                connectedDevice = null
                if (userStopped) {
                    notifyState("ERROR", "SPP 连接失败: ${e.message}（若未配对请先在系统蓝牙中配对）")
                } else {
                    notifyState("RECONNECTING", "SPP 连接失败: ${e.message}，自动重连中")
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
                val ins = s.inputStream
                while (!Thread.currentThread().isInterrupted) {
                    val n = ins.read(buf)
                    if (n > 0) {
                        val chunk = buf.copyOf(n)
                        mainHandler.post { listeners.forEach { it.onSppData(chunk) } }
                    }
                }
            } catch (_: IOException) {
                // 读取失败即链路断开
            } finally {
                // 仅当仍是当前链路的接收循环时才更新状态（避免切换目标时覆盖新连接）
                if (socket === s) {
                    connected = false
                    connectedDevice = null
                    if (userStopped) {
                        notifyState("DISCONNECTED", "SPP 连接已断开")
                    } else {
                        notifyState("RECONNECTING", "SPP 连接断开，自动重连中")
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
            socket?.outputStream?.write(bytes)
            socket?.outputStream?.flush()
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
        connectedDevice = null
        try {
            rxThread?.interrupt()
            socket?.close()
        } catch (_: Exception) {
        }
        socket = null
        connected = false
        if (wasUp) {
            notifyState("DISCONNECTED", "SPP 连接已断开")
        }
    }
}
