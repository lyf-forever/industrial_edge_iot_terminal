package com.indedge.terminal.app.ui

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import com.indedge.terminal.app.R
import com.indedge.terminal.app.ble.BleDevice
import com.indedge.terminal.app.ble.BleListener
import com.indedge.terminal.app.ble.BleManager
import com.indedge.terminal.app.databinding.FragmentBleBinding
import com.indedge.terminal.app.ui.adapters.BleDeviceAdapter
import com.indedge.terminal.app.util.ByteCodec
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * BLE 页：设备扫描（含 RSSI）、RSSI 实时曲线、GATT 连接与透传收发。
 * 契约默认 Nordic UART Service，可在“设置”页修改 UUID。
 */
class BleFragment : Fragment(), BleListener {

    private var _binding: FragmentBleBinding? = null
    private val binding get() = _binding!!

    private var selectedAddr: String? = null
    private var hexMode = false

    private val timeFmt = SimpleDateFormat("HH:mm:ss", Locale.getDefault())
    private var rxCount = 0L
    private var txCount = 0L

    private val deviceAdapter: BleDeviceAdapter? get() = _deviceAdapter
    private var _deviceAdapter: BleDeviceAdapter? = null

    private val enableBtLauncher = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { Toast.makeText(requireContext(), "蓝牙状态已更新", Toast.LENGTH_SHORT).show() }

    private val permLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { grants ->
        if (grants.values.all { it }) {
            BleManager.startScan()
        } else {
            Toast.makeText(requireContext(), "缺少蓝牙权限", Toast.LENGTH_SHORT).show()
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentBleBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        BleManager.init(requireContext())

        _deviceAdapter = BleDeviceAdapter(requireContext())
        binding.lvDevices.adapter = _deviceAdapter
        binding.lvDevices.setOnItemClickListener { _, _, position, _ ->
            _deviceAdapter?.getItem(position)?.let { onDeviceTapped(it) }
        }
        // 页面重建：从扫描缓存恢复设备列表（无需重新扫描）
        seedDeviceList()

        binding.btnScan.setOnClickListener { toggleScan() }
        binding.btnEnableBt.setOnClickListener { ensureBluetoothEnabled() }
        binding.btnDisconnect.setOnClickListener {
            BleManager.disconnect()
            binding.tvGattState.text = getString(R.string.not_connected)
        }
        binding.btnSend.setOnClickListener { sendData() }
        binding.tbHex.setOnCheckedChangeListener { _, checked -> hexMode = checked }
        binding.btnClearRx.setOnClickListener {
            binding.tvRx.text = ""
            binding.rssiChart.clear()
            rxCount = 0
            txCount = 0
            updateCount()
        }
    }

    override fun onResume() {
        super.onResume()
        BleManager.register(this)
        // 页面重建/切回：同步当前扫描与连接状态（register 不推送历史状态）
        if (BleManager.scanning) {
            binding.btnScan.text = getString(R.string.ble_scan_stop)
            binding.tvScanState.text = getString(R.string.ble_scanning)
        }
        if (BleManager.connected) {
            binding.tvGattState.text = getString(R.string.connected)
            binding.tvGattDetail.text = BleManager.gattDevice?.address ?: ""
        }
    }

    override fun onPause() {
        BleManager.unregister(this)
        super.onPause()
    }

    override fun onDestroyView() {
        _deviceAdapter = null
        _binding = null
        super.onDestroyView()
    }

    private fun seedDeviceList() {
        val a = deviceAdapter ?: return
        for (d in BleManager.devices.values) {
            val exists = (0 until a.count)
                .any { a.getItem(it)!!.device.address == d.device.address }
            if (!exists) a.add(d)
        }
        a.sort { x, y -> y.rssi - x.rssi }
    }

    private fun ensureBluetoothEnabled() {
        if (!BleManager.isBluetoothEnabled()) {
            enableBtLauncher.launch(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE))
            return
        }
        Toast.makeText(requireContext(), "蓝牙已开启", Toast.LENGTH_SHORT).show()
    }

    private fun toggleScan() {
        if (!BleManager.isBluetoothEnabled()) {
            ensureBluetoothEnabled()
            return
        }
        if (!hasBlePermission()) {
            requestBlePermission()
            return
        }
        if (BleManager.scanning) BleManager.stopScan() else BleManager.startScan()
    }

    private fun hasBlePermission(): Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            ContextCompat.checkSelfPermission(requireContext(), Manifest.permission.BLUETOOTH_SCAN)
                == PackageManager.PERMISSION_GRANTED &&
                ContextCompat.checkSelfPermission(requireContext(), Manifest.permission.BLUETOOTH_CONNECT)
                == PackageManager.PERMISSION_GRANTED
        } else {
            ContextCompat.checkSelfPermission(requireContext(), Manifest.permission.ACCESS_FINE_LOCATION)
                == PackageManager.PERMISSION_GRANTED
        }
    }

    private fun requestBlePermission() {
        val perms = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
        } else {
            arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)
        }
        permLauncher.launch(perms)
    }

    private fun onDeviceTapped(d: BleDevice) {
        selectedAddr = d.device.address
        binding.rssiChart.clear()
        binding.rssiChart.addSample(d.rssi)
        BleManager.stopScan()
        BleManager.connect(d.device, requireContext())
    }

    private fun sendData() {
        val text = binding.etTx.text.toString()
        if (text.isEmpty()) return
        val bytes = ByteCodec.encodeSend(text, hexMode)
        if (bytes.isEmpty()) {
            Toast.makeText(requireContext(), "HEX 格式错误", Toast.LENGTH_SHORT).show()
            return
        }
        if (!BleManager.connected) {
            Toast.makeText(requireContext(), "未连接设备", Toast.LENGTH_SHORT).show()
            return
        }
        BleManager.send(bytes)
        txCount += bytes.size
        updateCount()
        binding.etTx.text?.clear()
    }

    private fun updateCount() {
        binding.tvCount.text = getString(R.string.io_count, rxCount, txCount)
    }

    // ================= BleListener =================

    override fun onDeviceFound(d: BleDevice) {
        val a = deviceAdapter ?: return
        val idx = (0 until a.count)
            .firstOrNull { a.getItem(it)!!.device.address == d.device.address }
        if (idx == null) {
            a.add(d)
        } else {
            a.remove(a.getItem(idx))
            a.insert(d, idx)
        }
        // 按 RSSI 降序排列（信号强的置顶）
        a.sort { x, y -> y.rssi - x.rssi }
        // 扫描态：实时刷新选中/最近设备的 RSSI 曲线
        if (selectedAddr == null || d.device.address == selectedAddr) {
            binding.rssiChart.addSample(d.rssi)
        }
    }

    override fun onScanState(active: Boolean) {
        binding.btnScan.text = if (active) getString(R.string.ble_scan_stop) else getString(R.string.ble_scan_start)
        binding.tvScanState.text = if (active) {
            getString(R.string.ble_scanning)
        } else {
            getString(R.string.ble_scan_idle, BleManager.devices.size)
        }
    }

    override fun onGattState(state: String, detail: String) {
        val text = when (state) {
            "CONNECTING" -> getString(R.string.connecting)
            "CONNECTED" -> getString(R.string.connected)
            "READY" -> getString(R.string.gatt_ready)
            "DISCONNECTED" -> getString(R.string.disconnected)
            else -> getString(R.string.error_state)
        }
        binding.tvGattState.text = text
        binding.tvGattDetail.text = detail
    }

    override fun onData(bytes: ByteArray) {
        rxCount += bytes.size
        updateCount()
        val line = "[${timeFmt.format(Date())}] ${ByteCodec.decodeShow(bytes, hexMode)}"
        val old = binding.tvRx.text.toString()
        binding.tvRx.text = if (old.isEmpty()) line else "$old\n$line"
        binding.scrollRx.post { binding.scrollRx.fullScroll(View.FOCUS_DOWN) }
    }

    override fun onRssi(rssi: Int) {
        binding.rssiChart.addSample(rssi)
    }
}
