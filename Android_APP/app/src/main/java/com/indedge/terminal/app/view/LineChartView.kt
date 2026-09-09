package com.indedge.terminal.app.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Path
import android.util.AttributeSet
import android.view.View
import androidx.core.content.ContextCompat
import com.indedge.terminal.app.R
import kotlin.math.max

/**
 * 通用历史折线图：支持多条数据序列（各自独立保存），
 * 同一时刻显示其中一条（Spinner 切换），Y 轴自动缩放，右对齐滚动窗口。
 */
class LineChartView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    class Series(val name: String, val color: Int, val maxPoints: Int) {
        val points = ArrayDeque<Float>()
    }

    private val seriesList = mutableListOf<Series>()
    private var visibleIndex = 0

    private val paintGrid = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.chart_grid)
        strokeWidth = 2f
    }
    private val paintLine = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        strokeWidth = 4f
        style = Paint.Style.STROKE
        strokeJoin = Paint.Join.ROUND
        strokeCap = Paint.Cap.ROUND
    }
    private val paintText = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.chart_axis)
        textSize = 24f
    }
    private val paintValue = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.chart_axis)
        textSize = 28f
    }

    /** 注册一条序列，返回索引 */
    fun addSeries(name: String, color: Int, maxPoints: Int = 60): Int {
        seriesList.add(Series(name, color, maxPoints))
        return seriesList.size - 1
    }

    /** 序列名称列表（供 Spinner） */
    fun seriesNames(): List<String> = seriesList.map { it.name }

    /** 切换当前显示序列 */
    fun showSeries(index: Int) {
        if (index in seriesList.indices) {
            visibleIndex = index
            invalidate()
        }
    }

    /** 追加数据点 */
    fun addPoint(index: Int, value: Float) {
        val s = seriesList.getOrNull(index) ?: return
        s.points.addLast(value)
        while (s.points.size > s.maxPoints) s.points.removeFirst()
        if (index == visibleIndex) invalidate()
    }

    /** 用历史数据回填（页面重建时恢复曲线），超出窗口的旧点被裁剪 */
    fun setPoints(index: Int, points: List<Float>) {
        val s = seriesList.getOrNull(index) ?: return
        s.points.clear()
        for (v in points.takeLast(s.maxPoints)) {
            s.points.addLast(v)
        }
        invalidate()
    }

    fun clear() {
        seriesList.forEach { it.points.clear() }
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val w = width.toFloat()
        val h = height.toFloat()
        val padL = 64f
        val padR = 12f
        val padT = 12f
        val padB = 12f

        val s = seriesList.getOrNull(visibleIndex)
        if (s == null || s.points.isEmpty()) {
            canvas.drawText("等待数据…", padL, h / 2f, paintText)
            return
        }

        val vMin = s.points.min()
        val vMax = s.points.max()
        // 避免恒定值导致的零区间
        val span = max(vMax - vMin, 1f)
        val lo = vMin - span * 0.15f
        val hi = vMax + span * 0.15f

        fun yOf(v: Float): Float = padT + (hi - v) * (h - padT - padB) / (hi - lo)

        // 网格（4 档）与刻度
        for (i in 0..4) {
            val v = hi - (hi - lo) * i / 4f
            val y = yOf(v)
            canvas.drawLine(padL, y, w - padR, y, paintGrid)
            canvas.drawText(String.format("%.1f", v), 4f, y + 8f, paintText)
        }

        // 折线（右对齐滚动）
        val step = (w - padL - padR) / (s.maxPoints - 1).toFloat()
        val path = Path()
        var first = true
        s.points.forEachIndexed { i, v ->
            val x = w - padR - (s.points.size - 1 - i) * step
            val y = yOf(v)
            if (first) {
                path.moveTo(x, y)
                first = false
            } else {
                path.lineTo(x, y)
            }
        }
        paintLine.color = s.color
        canvas.drawPath(path, paintLine)

        // 图例 + 最新值
        val last = s.points.last()
        canvas.drawText("${s.name}: $last", w - padR - 320f, padT + 32f, paintValue)
    }
}
