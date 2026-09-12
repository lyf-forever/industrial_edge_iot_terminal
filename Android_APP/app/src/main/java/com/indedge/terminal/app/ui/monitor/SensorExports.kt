package com.indedge.terminal.app.ui.monitor

import android.content.Context
import android.widget.Toast
import androidx.fragment.app.Fragment
import com.indedge.terminal.app.R
import com.indedge.terminal.app.db.SensorStore
import com.indedge.terminal.app.util.CsvExport
import com.indedge.terminal.app.util.ShareUtils
import com.indedge.terminal.app.util.TimeFmt
import java.io.File

/**
 * 监控页数据导出：CSV 生成 + FileProvider 分享。
 * 从 StatusFragment 拆出（后台线程构建，主线程分享）。
 */
object SensorExports {

    /** 导出最近 5000 条传感器记录为 CSV 并分享 */
    fun exportCsv(context: Context, fragment: Fragment) {
        val act = fragment.activity ?: return
        Thread {
            val rows = SensorStore.queryRecent(5000)
            if (rows.isEmpty()) {
                act.runOnUiThread {
                    Toast.makeText(context, R.string.csv_export_empty, Toast.LENGTH_SHORT).show()
                }
                return@Thread
            }
            val content = CsvExport.build(rows)
            val dir = File(context.filesDir, "exports").apply { mkdirs() }
            val file = File(dir, "sensor_history_${TimeFmt.fileStamp()}.csv")
            val ok = runCatching { file.writeText(content) }.isSuccess
            act.runOnUiThread {
                if (ok) {
                    ShareUtils.shareFile(context, fragment, file, "text/csv")
                } else {
                    Toast.makeText(context, R.string.csv_export_fail, Toast.LENGTH_SHORT).show()
                }
            }
        }.start()
    }
}
