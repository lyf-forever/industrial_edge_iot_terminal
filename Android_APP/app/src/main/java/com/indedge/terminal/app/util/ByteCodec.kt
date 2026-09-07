package com.indedge.terminal.app.util

/**
 * 透传收发面板的编解码工具：HEX / ASCII 双向转换。
 */
object ByteCodec {

    fun toHex(bytes: ByteArray): String =
        bytes.joinToString(" ") { "%02X".format(it) }

    private fun hexToBytes(s: String): ByteArray {
        val clean = s.replace(" ", "").replace("\n", "")
        if (clean.length % 2 != 0 || clean.any { it.digitToIntOrNull(16) == null }) {
            return ByteArray(0)
        }
        return ByteArray(clean.length / 2) { i ->
            clean.substring(i * 2, i * 2 + 2).toInt(16).toByte()
        }
    }

    /** 编码发送内容；hex=true 时按十六进制串解析，非法输入返回空数组 */
    fun encodeSend(input: String, hex: Boolean): ByteArray =
        if (hex) hexToBytes(input) else input.toByteArray(Charsets.UTF_8)

    /** 解码接收内容用于显示 */
    fun decodeShow(bytes: ByteArray, hex: Boolean): String =
        if (hex) toHex(bytes) else String(bytes, Charsets.UTF_8)
}
