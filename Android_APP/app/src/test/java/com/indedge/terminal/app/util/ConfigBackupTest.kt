package com.indedge.terminal.app.util

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class ConfigBackupTest {

    private val fullValues = mapOf(
        Prefs.KEY_MQTT_URI to "tcp://broker.emqx.io:1883",
        Prefs.KEY_CLIENT_ID to "ind_edge_app_test",
        Prefs.KEY_BLE_SERVICE_UUID to "6E400001-B5A3-F393-E0A9-E50E24DCCA9E",
        Prefs.KEY_BLE_TX_UUID to "6E400002-B5A3-F393-E0A9-E50E24DCCA9E",
        Prefs.KEY_BLE_RX_UUID to "6E400003-B5A3-F393-E0A9-E50E24DCCA9E",
        Prefs.KEY_TCP_HOST to "192.168.4.1",
        Prefs.KEY_KEEP_ALIVE to "1"
    )

    @Test
    fun `roundtrip preserves all values`() {
        val json = ConfigBackup.buildJson(fullValues, 8080)
        val parsed = ConfigBackup.parse(json)
        assertEquals(fullValues[Prefs.KEY_MQTT_URI], parsed?.get(Prefs.KEY_MQTT_URI))
        assertEquals(fullValues[Prefs.KEY_CLIENT_ID], parsed?.get(Prefs.KEY_CLIENT_ID))
        assertEquals(fullValues[Prefs.KEY_BLE_SERVICE_UUID], parsed?.get(Prefs.KEY_BLE_SERVICE_UUID))
        assertEquals(fullValues[Prefs.KEY_TCP_HOST], parsed?.get(Prefs.KEY_TCP_HOST))
        assertEquals(fullValues[Prefs.KEY_KEEP_ALIVE], parsed?.get(Prefs.KEY_KEEP_ALIVE))
        assertEquals("8080", parsed?.get(Prefs.KEY_TCP_PORT))
    }

    @Test
    fun `missing values filled as empty string`() {
        val json = ConfigBackup.buildJson(emptyMap(), 1234)
        val parsed = ConfigBackup.parse(json)
        assertEquals("", parsed?.get(Prefs.KEY_MQTT_URI))
        assertEquals("1234", parsed?.get(Prefs.KEY_TCP_PORT))
    }

    @Test
    fun `version mismatch rejected`() {
        val json = ConfigBackup.buildJson(fullValues, 8080).replace("\"version\": 1", "\"version\": 99")
        assertNull(ConfigBackup.parse(json))
    }

    @Test
    fun `malformed json rejected`() {
        assertNull(ConfigBackup.parse("not a json"))
        assertNull(ConfigBackup.parse(""))
    }
}
