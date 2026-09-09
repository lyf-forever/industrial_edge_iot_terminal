package com.indedge.terminal.app.util

import com.indedge.terminal.app.db.SensorRecord
import java.util.Date
import java.util.Locale

/**
 * CSV 导出内容构建（纯函数，供导出分享与单元测试复用）。
 */
object CsvExport {

    const val HEADER = "时间,温度(℃),湿度(%),气体(ppm),CO2(ppm),气压(hPa)"

    /**
     * @param rows 记录（任意顺序，内部按时间升序输出）
     */
    fun build(rows: List<SensorRecord>): String {
        val sb = StringBuilder(HEADER)
        sb.append('\n')
        for (r in rows.sortedBy { it.ts }) {
            sb.append(TimeFmt.csvStamp(Date(r.ts)))
            sb.append(',').append(String.format(Locale.US, "%.1f", r.temp))
            sb.append(',').append(String.format(Locale.US, "%.1f", r.humid))
            sb.append(',').append(r.gas)
            sb.append(',').append(r.co2)
            sb.append(',').append(r.press)
            sb.append('\n')
        }
        return sb.toString()
    }
}
