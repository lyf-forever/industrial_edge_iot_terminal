package com.indedge.terminal.app.service

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import com.indedge.terminal.app.MainActivity
import com.indedge.terminal.app.R
import com.indedge.terminal.app.mqtt.MqttEventListener
import com.indedge.terminal.app.mqtt.MqttManager
import com.indedge.terminal.app.mqtt.MqttProtocol
import com.indedge.terminal.app.util.Prefs

/**
 * MQTT 后台保活前台服务：
 * 保证锁屏/退后台后进程存活、连接保持；常驻通知实时显示连接状态，
 * 点击通知回到 APP，可一键停止保活。
 */
class MqttForegroundService : Service(), MqttEventListener {

    companion object {
        private const val CHANNEL_ID = "mqtt_conn"
        private const val NOTIF_ID = 2001
        const val ACTION_STOP = "com.indedge.terminal.app.action.STOP_KEEPALIVE"

        fun start(context: Context) {
            // minSdk 26：startForegroundService 始终可用
            context.startForegroundService(Intent(context, MqttForegroundService::class.java))
        }

        fun stop(context: Context) {
            context.stopService(Intent(context, MqttForegroundService::class.java))
        }
    }

    private fun createChannel() {
        // minSdk 26：通知渠道始终可用
        val channel = NotificationChannel(
            CHANNEL_ID,
            getString(R.string.keep_alive_channel),
            NotificationManager.IMPORTANCE_LOW
        )
        getSystemService(NotificationManager::class.java)?.createNotificationChannel(channel)
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        createChannel()
        MqttManager.register(this)
        startInForeground(getString(R.string.keep_alive_starting))
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action == ACTION_STOP) {
            stopSelf()
            return START_NOT_STICKY
        }
        // 进程被系统杀掉后服务重启（START_STICKY）：按已保存配置自动恢复连接
        maybeAutoConnect()
        return START_STICKY
    }

    private fun maybeAutoConnect() {
        if (MqttManager.connected || MqttManager.connecting) return
        val uri = Prefs.str(this, Prefs.KEY_MQTT_URI, MqttProtocol.DEFAULT_URI)
        val clientId = Prefs.str(this, Prefs.KEY_CLIENT_ID, MqttProtocol.defaultClientId())
        MqttManager.connect(uri, clientId)
    }

    override fun onDestroy() {
        MqttManager.unregister(this)
        super.onDestroy()
    }

    // ================= MqttEventListener =================

    override fun onConnectionChanged(connected: Boolean, detail: String) {
        val text = if (connected) getString(R.string.keep_alive_connected, detail)
        else getString(R.string.keep_alive_disconnected, detail)
        updateNotification(text)
    }

    override fun onMessage(topic: String, payload: String) {
        // 保活服务不消费业务消息
    }

    // ================= 通知 =================

    private fun startInForeground(text: String) {
        val notif = buildNotification(text)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(NOTIF_ID, notif, ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC)
        } else {
            startForeground(NOTIF_ID, notif)
        }
    }

    private fun updateNotification(text: String) {
        val nm = getSystemService(NotificationManager::class.java) ?: return
        nm.notify(NOTIF_ID, buildNotification(text))
    }

    private fun buildNotification(text: String): Notification {
        val contentIntent = PendingIntent.getActivity(
            this, 0,
            Intent(this, MainActivity::class.java).apply {
                flags = Intent.FLAG_ACTIVITY_SINGLE_TOP
            },
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )
        val stopIntent = PendingIntent.getService(
            this, 1,
            Intent(this, MqttForegroundService::class.java).setAction(ACTION_STOP),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )
        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_monitor)
            .setContentTitle(getString(R.string.keep_alive_title))
            .setContentText(text)
            .setOngoing(true)
            .setContentIntent(contentIntent)
            .addAction(0, getString(R.string.keep_alive_stop), stopIntent)
            .build()
    }
}
