package com.indedge.terminal.app.util

import android.os.Handler
import android.os.Looper

/**
 * 断线重连调度器：指数退避 + 可注入调度器（便于 JVM 单测）。
 * 统一 MQTT / SPP / TCP 三链路的"何时重试"逻辑。
 */
class ReconnectController(
    private val baseMs: Long = 2_000L,
    private val maxMs: Long = 30_000L,
    private val scheduler: Scheduler,
    private val onAttempt: () -> Unit
) {

    /** 延时调度抽象（生产=主线程 Handler，测试=假调度器） */
    interface Scheduler {
        fun postDelayed(delayMs: Long, action: () -> Unit)
        fun cancelPending()
    }

    var attempts: Int = 0
        private set

    var active: Boolean = false
        private set

    /** 用户主动发起连接：进入保活态并清零退避 */
    fun start() {
        active = true
        attempts = 0
        scheduler.cancelPending()
    }

    /** 连接成功：清零退避并取消待执行重试 */
    fun reset() {
        attempts = 0
        scheduler.cancelPending()
    }

    /** 用户主动断开：停止一切自动重连 */
    fun stop() {
        active = false
        scheduler.cancelPending()
    }

    /** 安排下一次重试（仅保活态有效） */
    fun schedule() {
        if (!active) return
        scheduler.cancelPending()
        val delay = Backoff.nextDelayMs(attempts, baseMs, maxMs)
        attempts++
        scheduler.postDelayed(delay) { if (active) onAttempt() }
    }
}

/** 主线程 Handler 调度器（生产实现，每个 Controller 独占实例） */
class MainLooperScheduler : ReconnectController.Scheduler {

    private val handler = Handler(Looper.getMainLooper())
    private var pending: Runnable? = null

    override fun postDelayed(delayMs: Long, action: () -> Unit) {
        val r = Runnable { action() }
        pending = r
        handler.postDelayed(r, delayMs)
    }

    override fun cancelPending() {
        pending?.let { handler.removeCallbacks(it) }
        pending = null
    }
}
