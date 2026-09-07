package com.indedge.terminal.app.util

import android.content.Context
import android.content.Intent
import androidx.core.content.FileProvider
import androidx.fragment.app.Fragment
import com.indedge.terminal.app.R
import java.io.File

/**
 * 文件分享工具：经 FileProvider 以 ACTION_SEND 分享
 * 应用私有目录（files/exports）中的导出文件。
 */
object ShareUtils {

    fun shareFile(context: Context, fragment: Fragment, file: File, mime: String) {
        val uri = FileProvider.getUriForFile(
            context,
            "${context.packageName}.fileprovider",
            file
        )
        val intent = Intent(Intent.ACTION_SEND).apply {
            type = mime
            putExtra(Intent.EXTRA_STREAM, uri)
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        }
        fragment.startActivity(Intent.createChooser(intent, context.getString(R.string.export_title)))
    }
}
