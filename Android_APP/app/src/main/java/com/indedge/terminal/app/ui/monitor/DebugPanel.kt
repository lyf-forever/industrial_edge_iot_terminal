package com.indedge.terminal.app.ui.monitor

import android.content.Context
import android.widget.Toast
import com.indedge.terminal.app.R
import com.indedge.terminal.app.databinding.FragmentStatusBinding
import com.indedge.terminal.app.mqtt.MqttManager
import com.indedge.terminal.app.mqtt.MqttProtocol

/**
 * 监控页"调试发布"面板：任意主题发布 / 订阅的交互逻辑。
 * 从 StatusFragment 拆出，仅依赖 binding 与 Context。
 */
class DebugPanel(
    private val context: Context,
    private val binding: FragmentStatusBinding
) {

    private var subscribed = false

    fun attach() {
        binding.etDbgTopic.setText(MqttProtocol.TOPIC_CMD)
        binding.etDbgPayload.setText("{\"cmd\":\"status\"}")
        binding.btnDbgPublish.setOnClickListener { publish() }
        binding.btnDbgSubscribe.setOnClickListener { toggleSubscribe() }
    }

    private fun topic(): String =
        binding.etDbgTopic.text?.toString()?.trim().orEmpty()

    private fun publish() {
        val t = topic()
        if (t.isEmpty()) {
            toast(R.string.dbg_topic_required)
            return
        }
        if (!MqttManager.connected) {
            toast(R.string.not_connected)
            return
        }
        val payload = binding.etDbgPayload.text?.toString().orEmpty()
        val ok = MqttManager.publishRaw(t, payload, 1)
        toast(if (ok) R.string.published else R.string.publish_fail)
    }

    private fun toggleSubscribe() {
        val t = topic()
        if (t.isEmpty()) {
            toast(R.string.dbg_topic_required)
            return
        }
        if (!MqttManager.connected) {
            toast(R.string.not_connected)
            return
        }
        if (subscribed) {
            MqttManager.unsubscribeRaw(t)
            subscribed = false
            binding.btnDbgSubscribe.text = context.getString(R.string.dbg_subscribe)
        } else {
            MqttManager.subscribeRaw(t)
            subscribed = true
            binding.btnDbgSubscribe.text = context.getString(R.string.dbg_subscribed)
        }
    }

    private fun toast(res: Int) {
        Toast.makeText(context, res, Toast.LENGTH_SHORT).show()
    }
}
