package com.indedge.terminal.app.ble

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.BluetoothLeScanner
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Handler
import android.os.Looper
import com.indedge.terminal.app.util.Prefs
import java.util.UUID

/**
 * BLE 链路（GATT 透传）管理器。
 *
 * 契约约定（可在“设置”页修改 UUID）：
 *   服务  6E400001-B5A3-F393-E0A9-E50E24DCCA9E（Nordic UART Service）
 *   写特征 6E400002-...（APP -> 设备，write）
 *   通知特征 6E400003-...（设备 -> APP，notify）
 * 该契约与后续 ESP32-S3 GATT Server 固件对齐（设计指南 Phase 2）。
 */
object BleProfile {
    const val UUID_SERVICE = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
    const val UUID_TX_WRITE = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
    const val UUID_RX_NOTIFY = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
}

data class BleDevice(
    val device: BluetoothDevice,
    val name: String,
    val rssi: Int,
    val timestamp: Long
)

interface BleListener {
    /** 扫描发现设备（主线程） */
    fun onDeviceFound(d: BleDevice)
    /** 扫描状态变化 */
    fun onScanState(active: Boolean)
    /** GATT 状态：CONNECTING / CONNECTED / DISCONNECTED / 错误 */
    fun onGattState(state: String, detail: String)
    /** 收到透传数据（主线程） */
    fun onData(bytes: ByteArray)
    /** 远端 RSSI 读取回调（主线程，供曲线绘制） */
    fun onRssi(rssi: Int)
}

@SuppressLint("MissingPermission")
@SuppressLint("MissingPermission")
object BleManager {

    private val mainHandler = Handler(Looper.getMainLooper())
    private val listeners = mutableSetOf<BleListener>()

    private var adapter: BluetoothAdapter? = null
    private var scanner: BluetoothLeScanner? = null
    private var gatt: BluetoothGatt? = null
    private var appCtx: Context? = null

    private var txCh: BluetoothGattCharacteristic? = null
    private var rxCh: BluetoothGattCharacteristic? = null

    @Volatile
    var scanning = false
        private set

    @Volatile
    var connected = false
        private set

    /** 当前连接的 GATT 设备（供 UI 页面重建时恢复状态） */
    @Volatile
    var gattDevice: BluetoothDevice? = null
        private set

    /** 扫描结果缓存：地址 -> 设备 */
    val devices = LinkedHashMap<String, BleDevice>()

    /** RSSI 周期读取（连接态） */
    private val rssiRunnable = object : Runnable {
        override fun run() {
            try {
                gatt?.readRemoteRssi()
            } catch (_: Exception) {
            }
            if (connected) mainHandler.postDelayed(this, 1000)
        }
    }

    fun init(context: Context) {
        if (adapter != null) return
        appCtx = context.applicationContext
        val bm = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        adapter = bm.adapter
        scanner = adapter?.bluetoothLeScanner
    }

    fun isBluetoothEnabled(): Boolean = adapter?.isEnabled == true

    fun register(l: BleListener) = listeners.add(l)

    fun unregister(l: BleListener) = listeners.remove(l)

    private fun notifyGatt(state: String, detail: String) {
        mainHandler.post { listeners.forEach { it.onGattState(state, detail) } }
    }

    private fun notifyScan(active: Boolean) {
        scanning = active
        mainHandler.post { listeners.forEach { it.onScanState(active) } }
    }

    private val scanCallback = object : ScanCallback() {
        @SuppressLint("MissingPermission")
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val dev = result.device
            val name = result.scanRecord?.deviceName ?: dev.name ?: "未知设备"
            val d = BleDevice(dev, name, result.rssi, System.currentTimeMillis())
            devices[dev.address] = d
            mainHandler.post { listeners.forEach { it.onDeviceFound(d) } }
        }

        override fun onBatchScanResults(results: MutableList<ScanResult>) {
            for (r in results) onScanResult(0, r)
        }

        override fun onScanFailed(errorCode: Int) {
            notifyScan(false)
            notifyGatt("ERROR", "BLE 扫描失败 code=$errorCode")
        }
    }

    @SuppressLint("MissingPermission")
    fun startScan() {
        val s = scanner ?: return
        devices.clear()
        try {
            val settings = ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .build()
            s.startScan(null, settings, scanCallback)
            notifyScan(true)
        } catch (e: Exception) {
            notifyGatt("ERROR", "启动扫描失败: ${e.message}")
        }
    }

    @SuppressLint("MissingPermission")
    fun stopScan() {
        try {
            scanner?.stopScan(scanCallback)
        } catch (_: Exception) {
        }
        notifyScan(false)
    }

    /** 连接指定设备（若已在扫描，调用方应先停止扫描） */
    @SuppressLint("MissingPermission")
    fun connect(device: BluetoothDevice, context: Context) {
        disconnect()
        notifyGatt("CONNECTING", device.address)
        gattDevice = device
        try {
            gatt = device.connectGatt(context, false, gattCallback)
        } catch (e: Exception) {
            gattDevice = null
            notifyGatt("ERROR", "连接失败: ${e.message}")
        }
    }

    private val gattCallback = object : BluetoothGattCallback() {

        @Deprecated("Deprecated in Java")
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                connected = true
                notifyGatt("CONNECTED", gatt.device.address)
                gatt.discoverServices()
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                connected = false
                gattDevice = null
                txCh = null
                rxCh = null
                gatt.close()
                mainHandler.removeCallbacks(rssiRunnable)
                notifyGatt("DISCONNECTED", if (status == 0) "正常断开" else "断线 status=$status")
            }
        }

        @Deprecated("Deprecated in Java")
        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                notifyGatt("ERROR", "服务发现失败 status=$status")
                return
            }
            val serviceUuid = serviceUuidOf()
            val svc = try {
                gatt.getService(UUID.fromString(serviceUuid))
            } catch (e: IllegalArgumentException) {
                null
            }
            if (svc == null) {
                notifyGatt("ERROR", "未找到服务 $serviceUuid")
                return
            }
            try {
                txCh = svc.getCharacteristic(UUID.fromString(txUuidOf()))
                rxCh = svc.getCharacteristic(UUID.fromString(rxUuidOf()))
            } catch (e: IllegalArgumentException) {
                txCh = null
                rxCh = null
            }
            if (txCh == null || rxCh == null) {
                notifyGatt("ERROR", "未找到透传特征（请检查设置页 UUID）")
                return
            }
            enableNotify()
        }

        @Deprecated("Deprecated in Java")
        override fun onCharacteristicChanged(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic
        ) {
            val v = characteristic.value ?: return
            mainHandler.post { listeners.forEach { it.onData(v.copyOf()) } }
        }

        @Deprecated("Deprecated in Java")
        override fun onDescriptorWrite(
            gatt: BluetoothGatt,
            descriptor: BluetoothGattDescriptor,
            status: Int
        ) {
            notifyGatt("READY", if (status == BluetoothGatt.GATT_SUCCESS) {
                "透传通道就绪"
            } else {
                "通知使能失败 status=$status"
            })
            if (status == BluetoothGatt.GATT_SUCCESS) {
                mainHandler.post(rssiRunnable)
            }
        }

        @Deprecated("Deprecated in Java")
        override fun onReadRemoteRssi(gatt: BluetoothGatt, rssi: Int, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                mainHandler.post { listeners.forEach { it.onRssi(rssi) } }
            }
        }
    }

    @SuppressLint("MissingPermission")
    private fun enableNotify() {
        val g = gatt ?: return
        val rx = rxCh ?: return
        try {
            g.setCharacteristicNotification(rx, true)
            val cccd = rx.getDescriptor(CCCD_UUID)
            if (cccd != null) {
                cccd.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                g.writeDescriptor(cccd)
            }
        } catch (e: Exception) {
            notifyGatt("ERROR", "使能通知失败: ${e.message}")
        }
    }

    /** 发送透传数据到写特征 */
    fun send(bytes: ByteArray): Boolean {
        return try {
            val ch = txCh ?: return false
            ch.writeType = BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
            ch.value = bytes
            gatt?.writeCharacteristic(ch) ?: false
        } catch (e: Exception) {
            false
        }
    }

    @SuppressLint("MissingPermission")
    fun disconnect() {
        try {
            gatt?.disconnect()
            gatt?.close()
        } catch (_: Exception) {
        }
        gatt = null
        gattDevice = null
        txCh = null
        rxCh = null
        connected = false
        mainHandler.removeCallbacks(rssiRunnable)
    }

    // 契约 UUID 读取（支持设置页自定义）
    private fun serviceUuidOf(): String {
        val ctx = appCtx ?: return BleProfile.UUID_SERVICE
        return Prefs.str(ctx, Prefs.KEY_BLE_SERVICE_UUID, BleProfile.UUID_SERVICE)
    }

    private fun txUuidOf(): String {
        val ctx = appCtx ?: return BleProfile.UUID_TX_WRITE
        return Prefs.str(ctx, Prefs.KEY_BLE_TX_UUID, BleProfile.UUID_TX_WRITE)
    }

    private fun rxUuidOf(): String {
        val ctx = appCtx ?: return BleProfile.UUID_RX_NOTIFY
        return Prefs.str(ctx, Prefs.KEY_BLE_RX_UUID, BleProfile.UUID_RX_NOTIFY)
    }

    private val CCCD_UUID: UUID =
        UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
}
