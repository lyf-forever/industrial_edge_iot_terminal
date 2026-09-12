package com.indedge.terminal.app.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import com.indedge.terminal.app.R
import com.indedge.terminal.app.databinding.FragmentControlBinding
import com.indedge.terminal.app.mqtt.MqttManager
import com.indedge.terminal.app.util.Prefs
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
        binding.btnAddTemplate.setOnClickListener { saveTemplate() }

        vm.connection.observe(viewLifecycleOwner) { st ->
            binding.tvConnState.text = if (st.connected) "已连接" else "未连接"
            binding.tvConnDetail.text = st.detail
        }
        vm.cmdEvent.observe(viewLifecycleOwner) { refreshCmdHistory() }
        vm.otaState.observe(viewLifecycleOwner) { st ->
            if (st == null) return@observe
            updateOtaUi(st)
        }

        refreshCmdHistory()
        renderTemplates()
    }

    /** OTA 进度/结果展示（契约 6.1 状态机） */
    private fun updateOtaUi(st: AppViewModel.OtaState) {
        val text = when (st.state) {
            0 -> "OTA：待下载"
            1 -> if (st.pct >= 0) "OTA：下载中 ${st.pct}%" else "OTA：下载中…"
            2 -> "OTA：校验并切换分区…"
            3 -> "OTA：升级完成，设备即将重启"
            else -> "OTA：失败（${st.detail}）"
        }
        binding.tvOtaState.text = text
        when {
            st.state == 1 && st.pct >= 0 -> {
                binding.pbOta.visibility = View.VISIBLE
                binding.pbOta.isIndeterminate = false
                binding.pbOta.progress = st.pct
            }
            st.state == 1 -> {
                binding.pbOta.visibility = View.VISIBLE
                binding.pbOta.isIndeterminate = true
            }
            st.state == 2 -> {
                binding.pbOta.visibility = View.VISIBLE
                binding.pbOta.isIndeterminate = true
            }
            else -> binding.pbOta.visibility = View.GONE
        }
    }

    // ================= 命令模板 =================

    private fun loadTemplates(): List<String> {
        val raw = Prefs.str(requireContext(), Prefs.KEY_CMD_TEMPLATES, "[]")
        return runCatching {
            val arr = org.json.JSONArray(raw)
            (0 until arr.length()).map { arr.getString(it) }
        }.getOrDefault(emptyList())
    }

    private fun saveTemplates(list: List<String>) {
        val arr = org.json.JSONArray()
        list.forEach { arr.put(it) }
        Prefs.setStr(requireContext(), Prefs.KEY_CMD_TEMPLATES, arr.toString())
        renderTemplates()
    }

    private fun saveTemplate() {
        val text = binding.etCmdTemplate.text?.toString()?.trim().orEmpty()
        if (text.isEmpty()) return
        val list = loadTemplates().toMutableList()
        if (text !in list) {
            if (list.size >= 10) {
                Toast.makeText(requireContext(), R.string.template_limit, Toast.LENGTH_SHORT).show()
                return
            }
            list.add(text)
            saveTemplates(list)
            binding.etCmdTemplate.text?.clear()
            Toast.makeText(requireContext(), R.string.template_added, Toast.LENGTH_SHORT).show()
        } else {
            Toast.makeText(requireContext(), R.string.template_exists, Toast.LENGTH_SHORT).show()
        }
    }

    private fun renderTemplates() {
        val container = binding.llTemplates
        container.removeAllViews()
        for (tpl in loadTemplates()) {
            val btn = Button(requireContext()).apply {
                text = tpl
                textSize = 13f
                isAllCaps = false
                setOnClickListener { sendCommand(tpl) }
                setOnLongClickListener {
                    saveTemplates(loadTemplates().filterNot { it == tpl })
                    Toast.makeText(requireContext(), R.string.template_removed, Toast.LENGTH_SHORT).show()
                    true
                }
            }
            container.addView(btn)
        }
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
