package com.indedge.terminal.app.db

import android.content.ContentValues
import android.content.Context
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper
import java.util.concurrent.Executors

/**
 * 传感器历史数据本地落盘（SQLite），落实设计文档"数据可追溯"。
 * 后台线程写入，避免阻塞主线程；查询接口用于离线回看。
 */

data class SensorRecord(
    val ts: Long,
    val temp: Float,
    val humid: Float,
    val gas: Int,
    val co2: Int,
    val press: Int
)

class SensorDb(context: Context) : SQLiteOpenHelper(context, "sensor_history.db", null, 1) {

    override fun onCreate(db: SQLiteDatabase) {
        db.execSQL(
            "CREATE TABLE history (" +
                "ts INTEGER PRIMARY KEY, " +
                "temp REAL, humid REAL, " +
                "gas INTEGER, co2 INTEGER, press INTEGER)"
        )
    }

    override fun onUpgrade(db: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
        // v1 无升级逻辑
    }

    fun insert(r: SensorRecord) {
        val v = ContentValues().apply {
            put("ts", r.ts)
            put("temp", r.temp)
            put("humid", r.humid)
            put("gas", r.gas)
            put("co2", r.co2)
            put("press", r.press)
        }
        writableDatabase.insertWithOnConflict("history", null, v, SQLiteDatabase.CONFLICT_REPLACE)
    }

    fun queryRecent(limit: Int): List<SensorRecord> {
        val rows = mutableListOf<SensorRecord>()
        readableDatabase.query(
            "history", null, null, null, null, null, "ts DESC", limit.toString()
        ).use { c ->
            while (c.moveToNext()) {
                rows.add(
                    SensorRecord(
                        ts = c.getLong(c.getColumnIndexOrThrow("ts")),
                        temp = c.getFloat(c.getColumnIndexOrThrow("temp")),
                        humid = c.getFloat(c.getColumnIndexOrThrow("humid")),
                        gas = c.getInt(c.getColumnIndexOrThrow("gas")),
                        co2 = c.getInt(c.getColumnIndexOrThrow("co2")),
                        press = c.getInt(c.getColumnIndexOrThrow("press"))
                    )
                )
            }
        }
        return rows
    }

    fun clear() {
        writableDatabase.delete("history", null, null)
    }

    /** 删除早于指定时间戳的记录（自动过期清理） */
    fun deleteOlderThan(ts: Long) {
        writableDatabase.delete("history", "ts < ?", arrayOf(ts.toString()))
    }
}

/** 进程内单例：初始化一次，写入走单线程队列 */
object SensorStore {

    /** 历史数据保留天数（超出自动清理） */
    const val RETENTION_DAYS = 30

    private var helper: SensorDb? = null
    private val executor = Executors.newSingleThreadExecutor()

    fun init(context: Context) {
        if (helper == null) {
            helper = SensorDb(context.applicationContext)
            // 启动时清理过期数据
            val cutoff = System.currentTimeMillis() - RETENTION_DAYS * 24L * 3600L * 1000L
            executor.execute { helper?.deleteOlderThan(cutoff) }
        }
    }

    fun insert(r: SensorRecord) {
        val h = helper ?: return
        executor.execute { h.insert(r) }
    }

    fun queryRecent(limit: Int): List<SensorRecord> =
        helper?.queryRecent(limit) ?: emptyList()

    fun clear() {
        val h = helper ?: return
        executor.execute { h.clear() }
    }
}
