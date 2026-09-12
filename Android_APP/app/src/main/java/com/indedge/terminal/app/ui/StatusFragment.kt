package com.indedge.terminal.app.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import android.widget.TextView
import android.widget.Toast
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import com.indedge.terminal.app.R
import com.indedge.terminal.app.databinding.FragmentStatusBinding
import com.indedge.terminal.app.db.SensorStore
import com.indedge.terminal.app.ui.monitor.DebugPanel
import com.indedge.terminal.app.ui.monitor.MonitorDialogs
import com.indedge.terminal.app.ui.monitor.SensorExports
import java.util.Locale

/**
 * 数据监控页（视图装配 + 数据观察）。
 * 业务子域拆分：对话框见 [MonitorDialogs]、导出见 [SensorExports]、调试面板见 [DebugPanel]；
 * 数据源为活动级 [AppViewModel]，切页返回数据不丢失。
 */
class StatusFragment : Fragment() {

    private var _binding: FragmentStatusBinding? = null
    private val binding get() = _binding!!

    private val vm: AppViewModel by activityViewModels()
    private var debugPanel: DebugPanel? = null
    private var fwVer: String? = null

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentStatusBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        setupChart()
        setupActions()
        observeViewModel()
        restoreUiState()
    }

    // ================= 历史曲线 =================

    private fun setupChart() {
        val names = arrayOf(
            getString(R.string.metric_temp),
            getString(R.string.metric_humid),
            getString(R.string.metric_gas),
            getString(R.string.metric_co2),
            getString(R.string.metric_press)
        )
        val colors = intArrayOf(
            0xFFE53935.toInt(),  // 温度
            0xFF1E88E5.toInt(),  // 湿度
            0xFFFF8F00.toInt(),  // 气体
            0xFF8E24AA.toInt(),  // CO2
            0xFF43A047.toInt()   // 气压
        )
        for (i in names.indices) {
            binding.historyChart.addSeries(names[i], colors[i], AppViewModel.SERIES_MAX)
            // 页面重建：从 ViewModel 回填历史点
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
    }

    // ================= 按钮装配 =================

    private fun setupActions() {
        binding.btnClearLog.setOnClickListener {
            binding.tvRaw.text = ""
            vm.clearLogs()
        }
        binding.btnQueryHistory.setOnClickListener { MonitorDialogs.showHistory(requireContext()) }
        binding.btnStats.setOnClickListener { MonitorDialogs.showStats(requireContext()) }
        binding.btnExportCsv.setOnClickListener { SensorExports.exportCsv(requireContext(), this) }
        binding.btnClearHistory.setOnClickListener {
            SensorStore.clear()
            vm.clearSeries()
            binding.historyChart.clear()
            Toast.makeText(requireContext(), R.string.history_cleared, Toast.LENGTH_SHORT).show()
        }
        binding.btnClearAlarms.setOnClickListener {
            vm.clearAlarms()
            binding.tvAlarm.text = getString(R.string.no_alarm)
            binding.alarmListContainer.removeAllViews()
        }
        binding.btnAlarmSettings.setOnClickListener {
            MonitorDialogs.showAlarmSettings(requireContext())
        }
        debugPanel = DebugPanel(requireContext(), binding).also { it.attach() }
    }

    // ================= 数据观察 =================

    private fun observeViewModel() {
        vm.connection.observe(viewLifecycleOwner) { st ->
            binding.tvConnState.text = getString(if (st.connected) R.string.connected else R.string.not_connected)
            binding.tvConnDetail.text = st.detail
            binding.tvConnState.setTextColor(
                ContextCompat.getColor(
                    requireContext(),
                    if (st.connected) R.color.green_online else R.color.red
                )
            )
        }
        vm.online.observe(viewLifecycleOwner) { online ->
            binding.tvDeviceOnline.text = getString(
                if (online) R.string.device_online else R.string.device_offline
            )
            binding.tvDeviceOnline.setTextColor(
                ContextCompat.getColor(
                    requireContext(),
                    if (online) R.color.green_online else R.color.red
                )
            )
        }
        vm.hbLine.observe(viewLifecycleOwner) { line ->
            binding.tvLastHb.text = if (fwVer.isNullOrEmpty()) line else "$line · FW $fwVer"
        }
        vm.deviceVersion.observe(viewLifecycleOwner) { v ->
            fwVer = v
            val base = vm.hbLine.value ?: ""
            binding.tvLastHb.text = if (v.isNullOrEmpty()) base else "$base · FW $v"
        }
        vm.sensorLatest.observe(viewLifecycleOwner) { d ->
            if (d == null) return@observe
            binding.tvTemp.text = String.format(Locale.US, "%.1f ℃", d.temp)
            binding.tvHumid.text = String.format(Locale.US, "%.1f %%", d.humid)
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

    override fun onDestroyView() {
        debugPanel = null
        _binding = null
        super.onDestroyView()
    }
}
