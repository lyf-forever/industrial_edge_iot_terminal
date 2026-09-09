package com.indedge.terminal.app.util

import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * 时间/时长格式化（纯函数，供 UI 与单元测试复用）。
 */
object TimeFmt {

    private val fileFmt = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.getDefault())
    private val rowFmt = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault())

    /** 秒 -> HH:mm:ss */
    fun formatUptime(seconds: Long): String {
        val h = seconds / 3600
        val m = seconds % 3600 / 60
        val s = seconds % 60
        return String.format("%02d:%02d:%02d", h, m, s)
    }

    /** 导出文件名时间戳 */
    fun fileStamp(date: Date = Date()): String = fileFmt.format(date)

    /** CSV 行时间戳 */
    fun csvStamp(date: Date = Date()): String = rowFmt.format(date)
}
