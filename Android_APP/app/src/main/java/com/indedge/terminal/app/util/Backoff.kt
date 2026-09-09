package com.indedge.terminal.app.util

import kotlin.math.min

/**
 * 指数退避延时计算（纯函数，供 MQTT/SPP/TCP 重连与单元测试复用）。
 */
object Backoff {

    /**
     * @param attempts 已重试次数（0 = 第一次调度）
     * @param baseMs   基础延时
     * @param maxMs    封顶延时
     */
    fun nextDelayMs(attempts: Int, baseMs: Long = 2_000L, maxMs: Long = 60_000L): Long {
        val exp = min(attempts.coerceAtLeast(0), 30)
        return min(baseMs shl exp, maxMs.coerceAtLeast(baseMs))
    }
}
