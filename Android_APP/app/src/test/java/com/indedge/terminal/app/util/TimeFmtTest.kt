package com.indedge.terminal.app.util

import org.junit.Assert.assertEquals
import org.junit.Test

class TimeFmtTest {

    @Test
    fun `uptime under one hour`() {
        assertEquals("00:00:59", TimeFmt.formatUptime(59))
    }

    @Test
    fun `uptime hours minutes seconds`() {
        assertEquals("01:02:03", TimeFmt.formatUptime(3723))
    }

    @Test
    fun `uptime multiple days`() {
        assertEquals("25:00:00", TimeFmt.formatUptime(25 * 3600L))
    }

    @Test
    fun `zero uptime`() {
        assertEquals("00:00:00", TimeFmt.formatUptime(0))
    }
}
