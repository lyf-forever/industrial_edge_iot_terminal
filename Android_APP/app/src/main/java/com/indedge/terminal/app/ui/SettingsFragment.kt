package com.indedge.terminal.app.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.CompoundButton
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import com.indedge.terminal.app.ble.BleProfile
import com.indedge.terminal.app.databinding.FragmentSettingsBinding
import com.indedge.terminal.app.mqtt.MqttManager
import com.indedge.terminal.app.mqtt.MqttProtocol
import com.indedge.terminal.app.net.TcpProfile
import com.indedge.terminal.app.service.MqttForegroundService
import com.indedge.terminal.app.util.ConfigBackup
import com.indedge.terminal.app.util.Prefs
import com.indedge.terminal.app.util.ShareUtils
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * 连接设置页：MQTT（Broker/Client ID）、BLE 契约 UUID、
 * LAN TCP 目标地址配置；后台保活开关（前台服务）；
 * 全量配置备份/导入（JSON 文件）。
 */
class SettingsFragment : Fragment() {

    private var _binding: FragmentSettingsBinding? = null
    private val binding get() = _binding!!

    private val vm: AppViewModel by activityViewModels()

    private val keepAliveListener = CompoundButton.OnCheckedChangeListener { _, checked ->
        Prefs.setStr(requireContext(), Prefs.KEY_KEEP_ALIVE, if (checked) "1" else "0")
        if (checked) {
            MqttForegroundService.start(requireContext())
            Toast.makeText(requireContext(), "后台保活已开启", Toast.LENGTH_SHORT).show()
        } else {
            MqttForegroundService.stop(requireContext())
            Toast.makeText(requireContext(), "后台保活已关闭", Toast.LENGTH_SHORT).show()
        }
    }

    private val importLauncher = registerForActivityResult(
        ActivityResultContracts.OpenDocument()
    ) { uri ->
        if (uri == null) return@registerForActivityResult
        try {
            val text = requireContext().contentResolver
                .openInputStream(uri)?.bufferedReader()?.readText().orEmpty()
            if (ConfigBackup.importJson(requireContext(), text)) {
                loadPrefs()
                // 刷新保活开关状态（临时摘除监听，避免误触发服务启停）
                binding.swKeepAlive.setOnCheckedChangeListener(null)
                binding.swKeepAlive.isChecked =
                    Prefs.str(requireContext(), Prefs.KEY_KEEP_ALIVE, "0") == "1"
                binding.swKeepAlive.setOnCheckedChangeListener(keepAliveListener)
                // 提示连接参数变更需重连生效
                val msg = if (MqttManager.connected) {
                    R.string.cfg_import_ok_connected
                } else {
                    R.string.cfg_import_ok
                }
                Toast.makeText(requireContext(), msg, Toast.LENGTH_LONG).show()
            } else {
                Toast.makeText(requireContext(), R.string.cfg_import_fail, Toast.LENGTH_SHORT).show()
            }
        } catch (_: Exception) {
            Toast.makeText(requireContext(), R.string.cfg_read_fail, Toast.LENGTH_SHORT).show()
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentSettingsBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        loadPrefs()
        binding.tvVersion.text = "v0.5.0 · MQTT 3.1.1 / BLE GATT / SPP / TCP"

        binding.btnToggleConnect.setOnClickListener {
            if (MqttManager.connected) {
                MqttManager.disconnect()
            } else {
                savePrefs()
                MqttManager.connect(
                    binding.etBrokerUri.text?.toString()?.trim().orEmpty(),
                    binding.etClientId.text?.toString()?.trim().orEmpty()
                )
            }
        }

        binding.btnSave.setOnClickListener {
            savePrefs()
            val msg = if (MqttManager.connected) {
                R.string.cfg_saved_connected
            } else {
                R.string.cfg_saved
            }
            Toast.makeText(requireContext(), msg, Toast.LENGTH_SHORT).show()
        }

        binding.swKeepAlive.isChecked = Prefs.str(requireContext(), Prefs.KEY_KEEP_ALIVE, "0") == "1"
        binding.swKeepAlive.setOnCheckedChangeListener(keepAliveListener)

        binding.btnExportCfg.setOnClickListener { exportConfig() }
        binding.btnImportCfg.setOnClickListener {
            importLauncher.launch(arrayOf("application/json", "application/octet-stream"))
        }

        vm.connection.observe(viewLifecycleOwner) { st ->
            binding.tvConnState.text = if (st.connected) "已连接" else "未连接"
            binding.tvConnDetail.text = st.detail
            binding.btnToggleConnect.text = if (st.connected) "断开连接" else "连接"
        }
    }

    override fun onDestroyView() {
        _binding = null
        super.onDestroyView()
    }

    // ================= 配置备份/恢复 =================

    private fun exportConfig() {
        val ctx = requireContext()
        try {
            val nameFmt = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.getDefault())
            val dir = File(ctx.filesDir, "exports").apply { mkdirs() }
            val file = File(dir, "config_backup_${nameFmt.format(Date())}.json")
            file.writeText(ConfigBackup.exportJson(ctx))
            ShareUtils.shareFile(ctx, this, file, "application/json")
        } catch (_: Exception) {
            Toast.makeText(requireContext(), R.string.cfg_read_fail, Toast.LENGTH_SHORT).show()
        }
    }

    // ================= 配置读写 =================

    private fun loadPrefs() {
        val ctx = requireContext()
        binding.etBrokerUri.setText(Prefs.str(ctx, Prefs.KEY_MQTT_URI, MqttProtocol.DEFAULT_URI))
        binding.etClientId.setText(Prefs.str(ctx, Prefs.KEY_CLIENT_ID, MqttProtocol.defaultClientId()))
        binding.etBleService.setText(Prefs.str(ctx, Prefs.KEY_BLE_SERVICE_UUID, BleProfile.UUID_SERVICE))
        binding.etBleTx.setText(Prefs.str(ctx, Prefs.KEY_BLE_TX_UUID, BleProfile.UUID_TX_WRITE))
        binding.etBleRx.setText(Prefs.str(ctx, Prefs.KEY_BLE_RX_UUID, BleProfile.UUID_RX_NOTIFY))
        binding.etTcpHost.setText(Prefs.str(ctx, Prefs.KEY_TCP_HOST, TcpProfile.DEFAULT_HOST))
        binding.etTcpPort.setText(Prefs.int(ctx, Prefs.KEY_TCP_PORT, TcpProfile.DEFAULT_PORT).toString())
    }

    private fun savePrefs() {
        val ctx = requireContext()
        Prefs.setStr(ctx, Prefs.KEY_MQTT_URI, binding.etBrokerUri.text?.toString()?.trim().orEmpty())
        Prefs.setStr(ctx, Prefs.KEY_CLIENT_ID, binding.etClientId.text?.toString()?.trim().orEmpty())
        Prefs.setStr(ctx, Prefs.KEY_BLE_SERVICE_UUID, binding.etBleService.text?.toString()?.trim().orEmpty())
        Prefs.setStr(ctx, Prefs.KEY_BLE_TX_UUID, binding.etBleTx.text?.toString()?.trim().orEmpty())
        Prefs.setStr(ctx, Prefs.KEY_BLE_RX_UUID, binding.etBleRx.text?.toString()?.trim().orEmpty())
        Prefs.setStr(ctx, Prefs.KEY_TCP_HOST, binding.etTcpHost.text?.toString()?.trim().orEmpty())
        Prefs.setInt(ctx, Prefs.KEY_TCP_PORT,
            binding.etTcpPort.text?.toString()?.toIntOrNull() ?: TcpProfile.DEFAULT_PORT)
    }
}
