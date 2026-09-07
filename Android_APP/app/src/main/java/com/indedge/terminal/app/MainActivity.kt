package com.indedge.terminal.app

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import com.indedge.terminal.app.databinding.ActivityMainBinding
import com.indedge.terminal.app.db.SensorStore
import com.indedge.terminal.app.mqtt.MqttManager
import com.indedge.terminal.app.ui.BleFragment
import com.indedge.terminal.app.ui.ControlFragment
import com.indedge.terminal.app.ui.PassThroughFragment
import com.indedge.terminal.app.ui.SettingsFragment
import com.indedge.terminal.app.ui.StatusFragment
import com.indedge.terminal.app.util.Notifier

/**
 * 工业边缘物联终端 APP 主界面。
 * 五个功能页：监控（MQTT）/ BLE / 透传（SPP+TCP）/ 控制（含 OTA）/ 设置。
 */
class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    private val notifPermLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { /* 结果不影响主流程，拒绝则告警通知静默 */ }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // 全局设施初始化
        Notifier.init(this)
        SensorStore.init(this)
        MqttManager.init(this)
        requestNotificationPermission()

        binding.bottomNav.setOnItemSelectedListener { item ->
            val fragment: Fragment = when (item.itemId) {
                R.id.nav_status -> StatusFragment()
                R.id.nav_ble -> BleFragment()
                R.id.nav_passthrough -> PassThroughFragment()
                R.id.nav_control -> ControlFragment()
                R.id.nav_settings -> SettingsFragment()
                else -> StatusFragment()
            }
            supportFragmentManager.beginTransaction()
                .replace(R.id.fragmentContainer, fragment)
                .commit()
            true
        }

        if (savedInstanceState == null) {
            binding.bottomNav.selectedItemId = R.id.nav_status
        }
    }

    private fun requestNotificationPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
            ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) !=
            PackageManager.PERMISSION_GRANTED
        ) {
            notifPermLauncher.launch(Manifest.permission.POST_NOTIFICATIONS)
        }
    }
}
