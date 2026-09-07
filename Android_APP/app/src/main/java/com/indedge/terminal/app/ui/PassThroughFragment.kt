package com.indedge.terminal.app.ui

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
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
import com.indedge.terminal.app.bt.SppListener
import com.indedge.terminal.app.bt.SppManager
import com.indedge.terminal.app.databinding.FragmentPassthroughBinding
import com.indedge.terminal.app.net.TcpListener
import com.indedge.terminal.app.net.TcpManager
import com.indedge.terminal.app.net.TcpProfile
import com.indedge.terminal.app.ui.adapters.BtDeviceAdapter
import com.indedge.terminal.app.util.ByteCodec
import com.indedge.terminal.app.util.Prefs
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * 透传页：经典蓝牙 SPP（BT24/HC05）+ LAN TCP 双通道切换，
 * 共用一套收发面板（HEX/ASCII 可切换）。
 */
class PassThroughFragment : Fragment(), SppListener, TcpListener {

    private var _binding: FragmentPassthroughBinding? = null
    private val binding get() = _binding!!

    private var hexMode = false
    private var channelSpp = true
    private var discovering = false
    private var receiverRegistered = false

    private val timeFmt = SimpleDateFormat("HH:mm:ss", Locale.getDefault())
    private var rxCount = 0L
    private var txCount = 0L

    private var btAdapter: BtDeviceAdapter? = null

    private val enableBtLauncher = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { }

    private val permLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { grants ->
        if (grants.values.all { it }) startDiscovery()
        else Toast.makeText(requireContext(), "缺少蓝牙权限", Toast.LENGTH_SHORT).show()
    }

    private val discoveryReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            when (intent.action) {
                BluetoothDevice.ACTION_FOUND -> {
                    val device = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                        intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE, BluetoothDevice::class.java)
                    } else {
                        @Suppress("DEPRECATION")
                        intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE)
                    } ?: return
                    addBtDevice(device)
                }
                BluetoothAdapter.ACTION_DISCOVERY_FINISHED -> {
                    // 发现结束：注销接收器并复位界面（discovering 可能已为 false，
                    // 但接收器仍注册着，必须在此注销防止泄漏/重复注册）
                    discoveryFinished()
                }
            }
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentPassthroughBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        btAdapter = BtDeviceAdapter(requireContext())
        binding.lvBtDevices.adapter = btAdapter
        binding.lvBtDevices.setOnItemClickListener { _, _, position, _ ->
            btAdapter?.getItem(position)?.let { onBtDeviceTapped(it) }
        }

        binding.channelGroup.addOnButtonCheckedListener { _, checkedId, isChecked ->
            if (!isChecked) return@addOnButtonCheckedListener
            channelSpp = checkedId == R.id.btnModeSpp
            binding.sppSection.visibility = if (channelSpp) View.VISIBLE else View.GONE
            binding.tcpSection.visibility = if (channelSpp) View.GONE else View.VISIBLE
        }

        binding.btnDiscover.setOnClickListener {
            if (!SppManager.isBluetoothEnabled()) {
                enableBtLauncher.launch(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE))
                return@setOnClickListener
            }
            if (!hasBlePermission()) {
                requestBlePermission()
                return@setOnClickListener
            }
            if (discovering) {
                stopDiscovery()
            } else {
                startDiscovery()
            }
        }

        binding.btnSppDisconnect.setOnClickListener { SppManager.disconnect() }

        binding.btnTcpConnect.setOnClickListener {
            if (TcpManager.connected) {
                TcpManager.disconnect()
            } else {
                connectTcp()
            }
        }

        binding.btnSend.setOnClickListener { sendData() }
        binding.tbHex.setOnCheckedChangeListener { _, checked -> hexMode = checked }
        binding.btnClearRx.setOnClickListener {
            binding.tvRx.text = ""
            rxCount = 0
            txCount = 0
            updateCount()
        }

        binding.etTcpHost.setText(Prefs.str(requireContext(), Prefs.KEY_TCP_HOST, TcpProfile.DEFAULT_HOST))
        binding.etTcpPort.setText(Prefs.int(requireContext(), Prefs.KEY_TCP_PORT, TcpProfile.DEFAULT_PORT).toString())
    }

    override fun onResume() {
        super.onResume()
        SppManager.register(this)
        TcpManager.register(this)
        // 读取已配对设备需要蓝牙权限（Android 12+）
        if (hasBlePermission()) loadPairedDevices()
    }

    override fun onPause() {
        stopDiscovery()
        SppManager.unregister(this)
        TcpManager.unregister(this)
        super.onPause()
    }    override fun onDestroyView() {
        btAdapter = null
        _binding = null
        super.onDestroyView()
    }

    // ================= 蓝牙发现 =================

    private fun loadPairedDevices() {
        if (!SppManager.isBluetoothEnabled()) return
        btAdapter?.clear()
        btAdapter?.connectedAddress = SppManager.connectedDevice?.address
        SppManager.pairedDevices().forEach { addBtDevice(it) }
        btAdapter?.notifyDataSetChanged()
    }

    private fun addBtDevice(device: BluetoothDevice) {
        val a = btAdapter ?: return
        val exists = (0 until a.count).any { a.getItem(it)!!.address == device.address }
        if (!exists) a.add(device)
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

    private fun startDiscovery() {
        if (receiverRegistered) {
            // 上次发现的广播仍在监听中（异常场景兜底），先清理再重启
            discoveryFinished()
        }
        val registered = runCatching {
            val filter = IntentFilter().apply {
                addAction(BluetoothDevice.ACTION_FOUND)
                addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED)
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                requireContext().registerReceiver(discoveryReceiver, filter, Context.RECEIVER_EXPORTED)
            } else {
                @Suppress("UnspecifiedRegisterReceiverFlag")
                requireContext().registerReceiver(discoveryReceiver, filter)
            }
        }.isSuccess
        if (!registered) {
            Toast.makeText(requireContext(), "注册广播失败", Toast.LENGTH_SHORT).show()
            return
        }
        receiverRegistered = true
        val started = SppManager.btAdapter()?.startDiscovery() == true
        if (started) {
            discovering = true
            binding.btnDiscover.text = getString(R.string.discover_stop)
            binding.tvSppState.text = getString(R.string.discovering)
        } else {
            unregisterDiscoveryReceiver()
            binding.tvSppState.text = getString(R.string.spp_idle)
        }
    }

    /** 注销发现广播（幂等） */
    private fun unregisterDiscoveryReceiver() {
        if (!receiverRegistered) return
        receiverRegistered = false
        runCatching { requireContext().unregisterReceiver(discoveryReceiver) }
    }

    /** 用户主动停止 / 页面暂停 / 系统发现结束：统一清理 */
    private fun stopDiscovery() {
        discovering = false
        unregisterDiscoveryReceiver()
        runCatching { SppManager.btAdapter()?.cancelDiscovery() }
        _binding?.let { b ->
            b.btnDiscover.text = getString(R.string.discover_devices)
        }
    }

    /** 系统广播发现结束：复位状态（含按钮文字） */
    private fun discoveryFinished() {
        discovering = false
        unregisterDiscoveryReceiver()
        runCatching { SppManager.btAdapter()?.cancelDiscovery() }
        _binding?.let { b ->
            b.btnDiscover.text = getString(R.string.discover_devices)
            b.tvSppState.text = getString(R.string.spp_idle)
        }
    }

    private fun onBtDeviceTapped(device: BluetoothDevice) {
        if (device.bondState != BluetoothDevice.BOND_BONDED) {
            Toast.makeText(requireContext(), "请先在系统蓝牙中完成配对后再次点击连接", Toast.LENGTH_LONG).show()
            runCatching { device.createBond() }
            return
        }
        if (SppManager.connecting) {
            Toast.makeText(requireContext(), R.string.link_busy, Toast.LENGTH_SHORT).show()
            return
        }
        stopDiscovery()
        // 内部处理目标切换：若已连接其他设备会自动断开旧链路
        SppManager.connect(device)
    }

    // ================= TCP =================

    private fun connectTcp() {
        val host = binding.etTcpHost.text.toString().trim()
        val port = binding.etTcpPort.text.toString().toIntOrNull() ?: TcpProfile.DEFAULT_PORT
        if (host.isEmpty()) {
            Toast.makeText(requireContext(), "请输入目标地址", Toast.LENGTH_SHORT).show()
            return
        }
        if (TcpManager.connected) {
            TcpManager.disconnect()
            return
        }
        if (TcpManager.connecting) {
            Toast.makeText(requireContext(), R.string.link_busy, Toast.LENGTH_SHORT).show()
            return
        }
        Prefs.setStr(requireContext(), Prefs.KEY_TCP_HOST, host)
        Prefs.setInt(requireContext(), Prefs.KEY_TCP_PORT, port)
        TcpManager.connect(host, port)
    }

    // ================= 收发 =================

    private fun sendData() {
        val text = binding.etTx.text.toString()
        if (text.isEmpty()) return
        val bytes = ByteCodec.encodeSend(text, hexMode)
        if (bytes.isEmpty()) {
            Toast.makeText(requireContext(), "HEX 格式错误", Toast.LENGTH_SHORT).show()
            return
        }
        val ok = if (channelSpp) SppManager.send(bytes) else TcpManager.send(bytes)
        if (!ok) {
            Toast.makeText(requireContext(), "发送失败：链路未连接", Toast.LENGTH_SHORT).show()
            return
        }
        txCount += bytes.size
        updateCount()
        binding.etTx.text?.clear()
    }

    private fun updateCount() {
        binding.tvCount.text = getString(R.string.io_count, rxCount, txCount)
    }

    private fun appendRx(bytes: ByteArray) {
        rxCount += bytes.size
        updateCount()
        val line = "[${timeFmt.format(Date())}] ${ByteCodec.decodeShow(bytes, hexMode)}"
        val old = binding.tvRx.text.toString()
        binding.tvRx.text = if (old.isEmpty()) line else "$old\n$line"
        binding.scrollRx.post { binding.scrollRx.fullScroll(View.FOCUS_DOWN) }
    }

    // ================= SppListener / TcpListener =================

    override fun onSppState(state: String, detail: String) {
        binding.tvSppState.text = when (state) {
            "CONNECTING" -> getString(R.string.connecting)
            "CONNECTED" -> getString(R.string.connected_fmt, detail)
            "RECONNECTING" -> getString(R.string.reconnect_state, detail)
            "DISCONNECTED" -> getString(R.string.disconnected)
            else -> detail
        }
        binding.btnDiscover.text = getString(R.string.discover_devices)
        // 同步设备列表“已连接”标记
        btAdapter?.connectedAddress = SppManager.connectedDevice?.address
        btAdapter?.notifyDataSetChanged()
    }

    override fun onSppData(bytes: ByteArray) {
        appendRx(bytes)
    }

    override fun onTcpState(state: String, detail: String) {
        binding.tvTcpState.text = when (state) {
            "CONNECTING" -> getString(R.string.connecting)
            "CONNECTED" -> getString(R.string.connected_fmt, detail)
            "RECONNECTING" -> getString(R.string.reconnect_state, detail)
            "DISCONNECTED" -> getString(R.string.disconnected)
            else -> detail
        }
        binding.btnTcpConnect.text = if (state == "CONNECTED") {
            getString(R.string.disconnect)
        } else {
            getString(R.string.connect)
        }
    }

    override fun onTcpData(bytes: ByteArray) {
        appendRx(bytes)
    }
}
