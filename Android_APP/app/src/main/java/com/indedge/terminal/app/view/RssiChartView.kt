package com.indedge.terminal.app.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Path
import android.util.AttributeSet
import android.view.View
import androidx.core.content.ContextCompat
import com.indedge.terminal.app.R
import kotlin.math.roundToInt

/**
 * RSSI 实时曲线视图（设计文档要求：BLE 扫描 RSSI 曲线）。
 * 范围 -100dBm ~ -30dBm，保留最近 maxSamples 个采样点，
 * 支持扫描态（扫描结果刷新）与连接态（readRemoteRssi 周期刷新）两种数据源。
 */
class RssiChartView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private val samples = ArrayDeque<Int>()
    private var maxSamples = 100

    private val paintGrid = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.chart_grid)
        strokeWidth = 2f
    }
    private val paintLine = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.primary)
        strokeWidth = 4f
        style = Paint.Style.STROKE
        strokeJoin = Paint.Join.ROUND
        strokeCap = Paint.Cap.ROUND
    }
    private val paintText = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.chart_axis)
        textSize = 26f
    }
    private val paintValue = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.chart_axis)
        textSize = 32f
    }

    /** 复用 Path，避免 onDraw 内分配 */
    private val path = Path()

    private val minRssi = -100f
    private val maxRssi = -30f

    fun addSample(rssi: Int) {
        samples.addLast(rssi.coerceIn(minRssi.toInt(), maxRssi.toInt()))
        while (samples.size > maxSamples) samples.removeFirst()
        invalidate()
    }

    fun clear() {
        samples.clear()
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val w = width.toFloat()
        val h = height.toFloat()
        val padL = 56f
        val padR = 12f
        val padT = 12f
        val padB = 12f

        fun yOf(r: Float): Float =
            padT + (maxRssi - r) * (h - padT - padB) / (maxRssi - minRssi)

        // 网格与刻度（每 10dB）
        var r = minRssi
        while (r <= maxRssi) {
            val y = yOf(r)
            canvas.drawLine(padL, y, w - padR, y, paintGrid)
            canvas.drawText("${r.roundToInt()}", 8f, y + 10f, paintText)
            r += 10f
        }

        if (samples.isEmpty()) {
            canvas.drawText("等待数据…", padL, h / 2f, paintText)
            return
        }

        // 折线（右对齐滚动）
        val step = (w - padL - padR) / (maxSamples - 1).toFloat()
        path.reset()
        var first = true
        samples.forEachIndexed { i, v ->
            val x = w - padR - (samples.size - 1 - i) * step
            val y = yOf(v.toFloat())
            if (first) {
                path.moveTo(x, y)
                first = false
            } else {
                path.lineTo(x, y)
            }
        }
        canvas.drawPath(path, paintLine)

        // 当前值
        val last = samples.last()
        canvas.drawText("$last dBm", w - padR - 200f, padT + 36f, paintValue)
    }
}
