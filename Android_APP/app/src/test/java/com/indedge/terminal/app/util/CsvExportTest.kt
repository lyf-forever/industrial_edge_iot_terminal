package com.indedge.terminal.app.util

import com.indedge.terminal.app.db.SensorRecord
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class CsvExportTest {

    @Test
    fun `header present`() {
        val csv = CsvExport.build(emptyList())
        assertEquals(CsvExport.HEADER + "\n", csv)
    }

    @Test
    fun `rows sorted ascending by time`() {
        val older = SensorRecord(1000L, 20.5f, 55f, 10, 400, 1013)
        val newer = SensorRecord(2000L, 21.5f, 56f, 12, 410, 1014)
        val csv = CsvExport.build(listOf(newer, older))
        val lines = csv.trim().split("\n")
        assertEquals(3, lines.size)
        assertTrue(lines[1].contains("20.5"))
        assertTrue(lines[2].contains("21.5"))
    }

    @Test
    fun `values separated by commas`() {
        val rec = SensorRecord(12345L, 22f, 60f, 100, 450, 1013)
        val csv = CsvExport.build(listOf(rec))
        val body = csv.trim().split("\n")[1]
        assertEquals(7, body.split(",").size)
        assertTrue(body.contains("100"))
        assertTrue(body.contains("450"))
    }
}
