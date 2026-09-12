package com.indedge.terminal.app.ui.monitor

import android.content.Context
import android.view.LayoutInflater
import android.widget.ArrayAdapter
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import com.indedge.terminal.app.R
import com.indedge.terminal.app.db.SensorStore
import com.indedge.terminal.app.util.AlarmPrefs
import com.indedge.terminal.app.util.Prefs
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * 监控页对话框集合：历史查询 / 24h 统计 / 告警设置。
 * 从 StatusFragment 拆出，全部基于 Context，可直接单测友好地复用。
 */
object MonitorDialogs {

    private val fullFmt = SimpleDateFormat("MM-dd HH:mm:ss", Locale.US)

    /** 历史记录（最近 50 条） */
    fun showHistory(context: Context) {
        val rows = SensorStore.queryRecent(50)
        if (rows.isEmpty()) {
            Toast.makeText(context, R.string.history_empty, Toast.LENGTH_SHORT).show()
            return
        }
        val items = rows.map { r ->
            context.getString(
                R.string.history_row,
                fullFmt.format(Date(r.ts)),
                String.format(Locale.US, "%.1f", r.temp),
                String.format(Locale.US, "%.1f", r.humid),
                r.gas, r.co2, r.press
            )
        }
        AlertDialog.Builder(context)
            .setTitle(R.string.history_query)
            .setAdapter(ArrayAdapter(context, android.R.layout.simple_list_item_1, items), null)
            .setPositiveButton(R.string.close, null)
            .show()
    }

    /** 近 24 小时按小时聚合统计 */
    fun showStats(context: Context) {
        val rows = SensorStore.queryHourlyStats(24)
        if (rows.isEmpty()) {
            Toast.makeText(context, R.string.stats_empty, Toast.LENGTH_SHORT).show()
            return
        }
        val hourFmt = SimpleDateFormat("dd日 HH:00", Locale.US)
        val items = rows.map { r ->
            context.getString(
                R.string.stats_row,
                hourFmt.format(Date(r.bucketMs)),
                String.format(Locale.US, "%.1f", r.avgTemp),
                String.format(Locale.US, "%.1f", r.avgHumid),
                r.maxGas
            )
        }
        AlertDialog.Builder(context)
            .setTitle(R.string.stats_title)
            .setAdapter(ArrayAdapter(context, android.R.layout.simple_list_item_1, items), null)
            .setPositiveButton(R.string.close, null)
            .show()
    }

    /** 告警通知设置（总开关 + 勿扰时段） */
    fun showAlarmSettings(context: Context) {
        val view = LayoutInflater.from(context)
            .inflate(R.layout.dialog_alarm_settings, null, false)
        val swNotify = view.findViewById<com.google.android.material.materialswitch.MaterialSwitch>(R.id.swNotifyAlarm)
        val swDnd = view.findViewById<com.google.android.material.materialswitch.MaterialSwitch>(R.id.swDnd)
        val etStart = view.findViewById<com.google.android.material.textfield.TextInputEditText>(R.id.etDndStart)
        val etEnd = view.findViewById<com.google.android.material.textfield.TextInputEditText>(R.id.etDndEnd)

        swNotify.isChecked = AlarmPrefs.notifyEnabled(context)
        swDnd.isChecked = AlarmPrefs.dndEnabled(context)
        etStart.setText(AlarmPrefs.dndStart(context))
        etEnd.setText(AlarmPrefs.dndEnd(context))

        AlertDialog.Builder(context)
            .setTitle(R.string.alarm_settings)
            .setView(view)
            .setPositiveButton(R.string.cfg_saved) { _, _ ->
                val start = etStart.text?.toString()?.trim().orEmpty()
                val end = etEnd.text?.toString()?.trim().orEmpty()
                if (!isHhmm(start) || !isHhmm(end)) {
                    Toast.makeText(context, R.string.dnd_bad_format, Toast.LENGTH_LONG).show()
                    return@setPositiveButton
                }
                Prefs.setStr(context, Prefs.KEY_ALARM_NOTIFY, if (swNotify.isChecked) "1" else "0")
                Prefs.setStr(context, Prefs.KEY_DND_ENABLED, if (swDnd.isChecked) "1" else "0")
                Prefs.setStr(context, Prefs.KEY_DND_START, start)
                Prefs.setStr(context, Prefs.KEY_DND_END, end)
            }
            .setNegativeButton(R.string.close, null)
            .show()
    }

    private fun isHhmm(s: String): Boolean =
        Regex("^([01]\\d|2[0-3]):[0-5]\\d$").matches(s)
}
