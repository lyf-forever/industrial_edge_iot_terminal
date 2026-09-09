package com.indedge.terminal.app.util

import org.junit.Assert.assertEquals
import org.junit.Test

class BackoffTest {

    @Test
    fun `first attempt uses base delay`() {
        assertEquals(2_000L, Backoff.nextDelayMs(0))
    }

    @Test
    fun `doubles exponentially`() {
        assertEquals(4_000L, Backoff.nextDelayMs(1))
        assertEquals(8_000L, Backoff.nextDelayMs(2))
        assertEquals(16_000L, Backoff.nextDelayMs(3))
    }

    @Test
    fun `caps at max`() {
        assertEquals(60_000L, Backoff.nextDelayMs(10))
        assertEquals(60_000L, Backoff.nextDelayMs(1000))
    }

    @Test
    fun `custom cap applies`() {
        assertEquals(30_000L, Backoff.nextDelayMs(20, baseMs = 2_000L, maxMs = 30_000L))
    }

    @Test
    fun `negative attempts treated as zero`() {
        assertEquals(2_000L, Backoff.nextDelayMs(-3))
    }
}
