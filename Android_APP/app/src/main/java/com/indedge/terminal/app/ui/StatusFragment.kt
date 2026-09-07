package com.indedge.terminal.app.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import com.indedge.terminal.app.R
import com.indedge.terminal.app.databinding.FragmentStatusBinding
import com.indedge.terminal.app.db.SensorStore
import com.indedge.terminal.app.mqtt.MqttManager
import com.indedge.terminal.app.mqtt.MqttProtocol
import com.indedge.terminal.app.util.ShareUtils
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * 数据监控页：实时数据、历史曲线、设备在线判定、告警历史、
 * MQTT 收发日志与调试发布面板。
 *
 * 页面展示数据全部来自 AppViewModel（活动级）：
 * 切页返回后曲线/日志/告警不丢失。
 */
class StatusFragment : Fragment() {

    private var _binding: FragmentStatusBinding? = null
    private val binding get() = _binding!!

    private val vm: AppViewModel by activityViewModels()

    private val fullFmt = SimpleDateFormat("MM-dd HH:mm:ss", Locale.getDefault())
    private var debugSubscribed = false

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentStatusBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        // 历史曲线：五条序列（数据源在 ViewModel）
        val names = arrayOf(
            getString(R.string.metric_temp),
            getString(R.string.metric_humid),
            getString(R.string.metric_gas),
            getString(R.string.metric_co2),
            getString(R.string.metric_press)
        )
        for (name in names) {
            binding.historyChart.addSeries(name, seriesColorOf(name), AppViewModel.SERIES_MAX)
        }
        // 页面重建：从 ViewModel 回填历史点
        for (i in 0 until AppViewModel.SERIES_COUNT) {
            binding.historyChart.setPoints(i, vm.seedSensorSeries(i))
        }

        binding.spMetric.adapter = ArrayAdapter(
            requireContext(),
            android.R.layout.simple_spinner_item,
            names
        ).also { it.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item) }
        binding.spMetric.onItemSelectedListener = object : android.widget.AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: android.widget.AdapterView<*>?, view: View?, position: Int, id: Long) {
                binding.historyChart.showSeries(position)
            }

            override fun onNothingSelected(parent: android.widget.AdapterView<*>?) {
            }
        }

        binding.btnClearLog.setOnClickListener {
            binding.tvRaw.text = ""
            vm.clearLogs()
        }
        binding.btnQueryHistory.setOnClickListener { showHistoryDialog() }
        binding.btnExportCsv.setOnClickListener { exportCsv() }
        binding.btnClearHistory.setOnClickListener {
            SensorStore.clear()
            vm.clearSeries()
            binding.historyChart.clear()
            Toast.makeText(requireContext(), "历史数据已清空", Toast.LENGTH_SHORT).show()
        }
        binding.btnDbgPublish.setOnClickListener { debugPublish() }
        binding.btnDbgSubscribe.setOnClickListener { toggleDebugSubscribe() }

        binding.etDbgTopic.setText(MqttProtocol.TOPIC_CMD)
        binding.etDbgPayload.setText("{\"cmd\":\"status\"}")

        observeViewModel()
        restoreUiState()
    }

    private fun observeViewModel() {
        vm.connection.observe(viewLifecycleOwner) { st ->
            binding.tvConnState.text = if (st.connected) "已连接" else "未连接"
            binding.tvConnDetail.text = st.detail
            binding.tvConnState.setTextColor(
                if (st.connected) 0xFF2E7D32.toInt() else 0xFFC62828.toInt()
            )
        }
        vm.online.observe(viewLifecycleOwner) { online ->
            binding.tvDeviceOnline.text = if (online) {
                getString(R.string.device_online)
            } else {
                getString(R.string.device_offline)
            }
            binding.tvDeviceOnline.setTextColor(
                if (online) 0xFF2E7D32.toInt() else 0xFFC62828.toInt()
            )
        }
        vm.hbLine.observe(viewLifecycleOwner) { line ->
            binding.tvLastHb.text = line
        }
        vm.sensorLatest.observe(viewLifecycleOwner) { d ->
            if (d == null) return@observe
            binding.tvTemp.text = String.format("%.1f ℃", d.temp)
            binding.tvHumid.text = String.format("%.1f %%", d.humid)
            binding.tvGas.text = "${d.gas} ppm"
            binding.tvCo2.text = "${d.co2} ppm"
            binding.tvPress.text = "${d.press} hPa"
        }
        vm.logEvent.observe(viewLifecycleOwner) {
            if (it != null) refreshLog()
        }
        vm.alarmEvent.observe(viewLifecycleOwner) { ev ->
            if (ev == null) return@observe
            binding.tvAlarm.text = "告警 #${ev.id} 等级 ${ev.level} 值 ${ev.value}"
            binding.alarmListContainer.addView(buildAlarmView(ev.line), 0)
        }
    }

    private fun restoreUiState() {
        // 重建日志与告警历史
        refreshLog()
        binding.alarmListContainer.removeAllViews()
        for (line in vm.alarmLines) {
            binding.alarmListContainer.addView(buildAlarmView(line))
        }
    }

    private fun refreshLog() {
        binding.tvRaw.text = vm.logText()
        binding.scrollRx.post { binding.scrollRx.fullScroll(View.FOCUS_DOWN) }
    }

    private fun buildAlarmView(line: String): TextView =
        TextView(requireContext()).apply {
            text = line
            textSize = 13f
            setTextColor(ContextCompat.getColor(requireContext(), R.color.orange))
            setPadding(0, 4, 0, 4)
        }

    private fun seriesColorOf(name: String): Int {
        return when (name) {
            getString(R.string.metric_temp) -> 0xFFE53935.toInt()
            getString(R.string.metric_humid) -> 0xFF1E88E5.toInt()
            getString(R.string.metric_gas) -> 0xFFFF8F00.toInt()
            getString(R.string.metric_co2) -> 0xFF8E24AA.toInt()
            else -> 0xFF43A047.toInt()
        }
    }

    override fun onDestroyView() {
        _binding = null
        super.onDestroyView()
    }

    // ================= 历史查询 / 导出 =================

    private fun showHistoryDialog() {
        val rows = SensorStore.queryRecent(50)
        if (rows.isEmpty()) {
            Toast.makeText(requireContext(), "暂无历史数据", Toast.LENGTH_SHORT).show()
            return
        }
        val items = rows.map { r ->
            getString(
                R.string.history_row,
                fullFmt.format(Date(r.ts)),
                String.format("%.1f", r.temp),
                String.format("%.1f", r.humid),
                r.gas, r.co2, r.press
            )
        }
        AlertDialog.Builder(requireContext())
            .setTitle(getString(R.string.history_query))
            .setAdapter(
                ArrayAdapter(
                    requireContext(),
                    android.R.layout.simple_list_item_1,
                    items
                ), null
            )
            .setPositiveButton(R.string.close, null)
            .show()
    }

    /** 导出最近 5000 条传感器记录为 CSV 并分享 */
    private fun exportCsv() {
        val ctx = requireContext()
        val act = requireActivity()
        Thread {
            val rows = SensorStore.queryRecent(5000)
            if (rows.isEmpty()) {
                act.runOnUiThread {
                    Toast.makeText(ctx, R.string.csv_export_empty, Toast.LENGTH_SHORT).show()
                }
                return@Thread
            }
            val rowFmt = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault())
            val nameFmt = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.getDefault())
            val sb = StringBuilder("时间,温度(℃),湿度(%),气体(ppm),CO2(ppm),气压(hPa)\n")
            for (r in rows.reversed()) {
                sb.append(rowFmt.format(Date(r.ts)))
                    .append(',').append(String.format("%.1f", r.temp))
                    .append(',').append(String.format("%.1f", r.humid))
                    .append(',').append(r.gas)
                    .append(',').append(r.co2)
                    .append(',').append(r.press)
                    .append('\n')
            }
            val dir = File(ctx.filesDir, "exports").apply { mkdirs() }
            val file = File(dir, "sensor_history_${nameFmt.format(Date())}.csv")
            val ok = runCatching { file.writeText(sb.toString()) }.isSuccess
            act.runOnUiThread {
                if (ok) {
                    ShareUtils.shareFile(ctx, this, file, "text/csv")
                } else {
                    Toast.makeText(ctx, R.string.csv_export_fail, Toast.LENGTH_SHORT).show()
                }
            }
        }.start()
    }

    // ================= 调试面板 =================

    private fun debugPublish() {
        val topic = binding.etDbgTopic.text?.toString()?.trim().orEmpty()
        val payload = binding.etDbgPayload.text?.toString().orEmpty()
        if (topic.isEmpty()) {
            Toast.makeText(requireContext(), "请输入主题", Toast.LENGTH_SHORT).show()
            return
        }
        if (!MqttManager.connected) {
            Toast.makeText(requireContext(), "未连接", Toast.LENGTH_SHORT).show()
            return
        }
        if (MqttManager.publishRaw(topic, payload, 1)) {
            Toast.makeText(requireContext(), "已发布", Toast.LENGTH_SHORT).show()
        } else {
            Toast.makeText(requireContext(), "发布失败", Toast.LENGTH_SHORT).show()
        }
    }

    private fun toggleDebugSubscribe() {
        val topic = binding.etDbgTopic.text?.toString()?.trim().orEmpty()
        if (topic.isEmpty()) {
            Toast.makeText(requireContext(), "请输入主题", Toast.LENGTH_SHORT).show()
            return
        }
        if (!MqttManager.connected) {
            Toast.makeText(requireContext(), "未连接", Toast.LENGTH_SHORT).show()
            return
        }
        if (debugSubscribed) {
            MqttManager.unsubscribeRaw(topic)
            debugSubscribed = false
            binding.btnDbgSubscribe.text = getString(R.string.dbg_subscribe)
        } else {
            MqttManager.subscribeRaw(topic)
            debugSubscribed = true
            binding.btnDbgSubscribe.text = getString(R.string.dbg_subscribed)
        }
    }
}
