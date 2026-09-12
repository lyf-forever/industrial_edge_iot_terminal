package com.indedge.terminal.app.util

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class AlarmPrefsTest {

    // ===== 普通窗口（开始 < 结束）=====

    @Test
    fun `normal window inside`() {
        // 08:00 - 18:00
        assertTrue(AlarmPrefs.inWindow(8 * 60, 8 * 60, 18 * 60))
        assertTrue(AlarmPrefs.inWindow(12 * 60, 8 * 60, 18 * 60))
        assertTrue(AlarmPrefs.inWindow(17 * 60 + 59, 8 * 60, 18 * 60))
    }

    @Test
    fun `normal window outside`() {
        assertFalse(AlarmPrefs.inWindow(7 * 60 + 59, 8 * 60, 18 * 60))
        assertFalse(AlarmPrefs.inWindow(18 * 60, 8 * 60, 18 * 60))
        assertFalse(AlarmPrefs.inWindow(0, 8 * 60, 18 * 60))
    }

    // ===== 跨零点窗口（开始 > 结束，如 22:00-07:00）=====

    @Test
    fun `wrap window inside`() {
        val start = 22 * 60
        val end = 7 * 60
        assertTrue(AlarmPrefs.inWindow(22 * 60, start, end))
        assertTrue(AlarmPrefs.inWindow(23 * 60 + 30, start, end))
        assertTrue(AlarmPrefs.inWindow(0, start, end))
        assertTrue(AlarmPrefs.inWindow(6 * 60 + 59, start, end))
    }

    @Test
    fun `wrap window outside`() {
        val start = 22 * 60
        val end = 7 * 60
        assertFalse(AlarmPrefs.inWindow(7 * 60, start, end))
        assertFalse(AlarmPrefs.inWindow(12 * 60, start, end))
        assertFalse(AlarmPrefs.inWindow(21 * 60 + 59, start, end))
    }

    @Test
    fun `zero width window excludes all`() {
        assertFalse(AlarmPrefs.inWindow(8 * 60, 8 * 60, 8 * 60))
        assertFalse(AlarmPrefs.inWindow(0, 8 * 60, 8 * 60))
    }
}
