package com.indedge.terminal.app.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import com.indedge.terminal.app.databinding.FragmentControlBinding
import com.indedge.terminal.app.mqtt.MqttManager
import org.json.JSONObject
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * 设备控制页：MQTT 下行命令 + OTA 触发。
 * 命令格式与固件 cloud_bridge.c 的 on_mqtt_msg 解析一致：
 *   {"cmd":"led","id":1,"state":1}
 *   {"cmd":"display","text":"..."}   -> EVT_CTRL_DISPLAY 跨核到 GD32 LCD
 *   {"cmd":"status"}
 *   {"cmd":"reboot"}
 *   {"cmd":"ota","url":"https://..."}
 * 命令历史由 AppViewModel 持有，切页不丢失。
 */
class ControlFragment : Fragment() {

    private var _binding: FragmentControlBinding? = null
    private val binding get() = _binding!!

    private val vm: AppViewModel by activityViewModels()

    private val timeFmt = SimpleDateFormat("HH:mm:ss", Locale.getDefault())

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentControlBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        binding.btnLed1On.setOnClickListener { sendLed(1, 1) }
        binding.btnLed1Off.setOnClickListener { sendLed(1, 0) }
        binding.btnLed2On.setOnClickListener { sendLed(2, 1) }
        binding.btnLed2Off.setOnClickListener { sendLed(2, 0) }
        binding.btnQueryStatus.setOnClickListener { sendCommand("{\"cmd\":\"status\"}") }
        binding.btnReboot.setOnClickListener { confirmReboot() }
        binding.btnSendDisplay.setOnClickListener { sendDisplay() }
        binding.btnStartOta.setOnClickListener { startOta() }
        binding.btnClearCmdLog.setOnClickListener {
            binding.tvCmdHistory.text = ""
            vm.clearCmds()
        }

        vm.connection.observe(viewLifecycleOwner) { st ->
            binding.tvConnState.text = if (st.connected) "已连接" else "未连接"
            binding.tvConnDetail.text = st.detail
        }
        vm.cmdEvent.observe(viewLifecycleOwner) { refreshCmdHistory() }

        refreshCmdHistory()
    }

    private fun refreshCmdHistory() {
        binding.tvCmdHistory.text = vm.cmdText()
        binding.scrollCmd.post { binding.scrollCmd.fullScroll(View.FOCUS_DOWN) }
    }

    override fun onDestroyView() {
        _binding = null
        super.onDestroyView()
    }

    private fun sendLed(id: Int, state: Int) {
        val json = JSONObject()
            .put("cmd", "led")
            .put("id", id)
            .put("state", state)
            .toString()
        sendCommand(json)
    }

    private fun sendDisplay() {
        val text = binding.etDisplayText.text?.toString()?.trim().orEmpty()
        if (text.isEmpty()) {
            Toast.makeText(requireContext(), "请输入显示内容", Toast.LENGTH_SHORT).show()
            return
        }
        val json = JSONObject()
            .put("cmd", "display")
            .put("text", text)
            .toString()
        sendCommand(json)
    }

    private fun startOta() {
        val url = binding.etFwUrl.text?.toString()?.trim().orEmpty()
        if (url.isEmpty()) {
            Toast.makeText(requireContext(), "请输入固件下载 URL", Toast.LENGTH_SHORT).show()
            return
        }
        if (!MqttManager.connected) {
            Toast.makeText(requireContext(), "未连接，请先在“设置”页连接", Toast.LENGTH_SHORT).show()
            return
        }
        val json = JSONObject().put("cmd", "ota").put("url", url).toString()
        if (MqttManager.publishCommand(json)) {
            vm.appendCmd("[${timeFmt.format(Date())}] → $json")
            binding.tvOtaState.text = "OTA 命令已下发，设备开始下载固件"
            Toast.makeText(requireContext(), "OTA 命令已下发", Toast.LENGTH_SHORT).show()
        } else {
            binding.tvOtaState.text = "OTA 命令下发失败"
        }
    }

    private fun sendCommand(json: String) {
        if (!MqttManager.connected) {
            Toast.makeText(requireContext(), "未连接，请先在“设置”页连接", Toast.LENGTH_SHORT).show()
            return
        }
        if (MqttManager.publishCommand(json)) {
            vm.appendCmd("[${timeFmt.format(Date())}] → $json")
            Toast.makeText(requireContext(), "命令已下发", Toast.LENGTH_SHORT).show()
        } else {
            vm.appendCmd("[${timeFmt.format(Date())}] 发送失败: $json")
            Toast.makeText(requireContext(), "命令下发失败", Toast.LENGTH_SHORT).show()
        }
    }

    private fun confirmReboot() {
        AlertDialog.Builder(requireContext())
            .setTitle("远程重启")
            .setMessage("确认重启设备？重启后链路将短暂断开。")
            .setPositiveButton("确认") { _, _ -> sendCommand("{\"cmd\":\"reboot\"}") }
            .setNegativeButton("取消", null)
            .show()
    }
}
