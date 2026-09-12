package com.indedge.terminal.app.util

import android.content.Context
import java.util.Calendar

/**
 * 告警通知策略（纯配置判定）：总开关 + 勿扰时段。
 */
object AlarmPrefs {

    const val DND_START_DEF = "22:00"
    const val DND_END_DEF = "07:00"

    fun notifyEnabled(context: Context): Boolean =
        Prefs.str(context, Prefs.KEY_ALARM_NOTIFY, "1") == "1"

    fun dndEnabled(context: Context): Boolean =
        Prefs.str(context, Prefs.KEY_DND_ENABLED, "0") == "1"

    fun dndStart(context: Context): String =
        Prefs.str(context, Prefs.KEY_DND_START, DND_START_DEF)

    fun dndEnd(context: Context): String =
        Prefs.str(context, Prefs.KEY_DND_END, DND_END_DEF)

    /** 是否处于勿扰时段（覆盖跨零点窗口，如 22:00-07:00） */
    fun inDndWindow(context: Context, nowMillis: Long = System.currentTimeMillis()): Boolean {
        if (!dndEnabled(context)) return false
        val cal = Calendar.getInstance().apply { timeInMillis = nowMillis }
        val minute = cal.get(Calendar.HOUR_OF_DAY) * 60 + cal.get(Calendar.MINUTE)
        val start = parseMinutes(dndStart(context)) ?: return false
        val end = parseMinutes(dndEnd(context)) ?: return false
        return inWindow(minute, start, end)
    }

    /**
     * 纯函数：分钟数是否落在 [startMin, endMin) 窗口内；
     * start > end 时按跨零点窗口处理（如 22:00-07:00）。
     */
    fun inWindow(minuteOfDay: Int, startMin: Int, endMin: Int): Boolean {
        return if (startMin <= endMin) {
            minuteOfDay in startMin until endMin
        } else {
            minuteOfDay >= startMin || minuteOfDay < endMin
        }
    }

    /** 当前是否允许推送告警通知 */
    fun shouldNotify(context: Context): Boolean =
        notifyEnabled(context) && !inDndWindow(context)

    private fun parseMinutes(hhmm: String): Int? {
        val parts = hhmm.trim().split(":")
        if (parts.size != 2) return null
        val h = parts[0].toIntOrNull() ?: return null
        val m = parts[1].toIntOrNull() ?: return null
        if (h !in 0..23 || m !in 0..59) return null
        return h * 60 + m
    }
}
