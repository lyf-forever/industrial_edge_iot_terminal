package com.indedge.terminal.app.util

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class ReconnectControllerTest {

    /** 假调度器：记录延时，可手动触发待执行动作 */
    private class FakeScheduler : ReconnectController.Scheduler {
        val delays = mutableListOf<Long>()
        var pending: (() -> Unit)? = null

        override fun postDelayed(delayMs: Long, action: () -> Unit) {
            delays.add(delayMs)
            pending = action
        }

        override fun cancelPending() {
            pending = null
        }
    }

    private fun newController(
        scheduler: FakeScheduler,
        onAttempt: () -> Unit = {}
    ) = ReconnectController(2_000L, 30_000L, scheduler, onAttempt)

    @Test
    fun `inactive controller ignores schedule`() {
        val s = FakeScheduler()
        val c = newController(s)
        c.schedule()
        assertTrue(s.delays.isEmpty())
        assertFalse(c.active)
    }

    @Test
    fun `backoff doubles until cap`() {
        val s = FakeScheduler()
        val c = newController(s)
        c.start()
        c.schedule()
        c.schedule()
        c.schedule()
        c.schedule()
        c.schedule()
        assertEquals(listOf(2_000L, 4_000L, 8_000L, 16_000L, 30_000L), s.delays)
        assertEquals(5, c.attempts)
    }

    @Test
    fun `reset clears attempts and cancels pending`() {
        val s = FakeScheduler()
        val c = newController(s)
        c.start()
        c.schedule()
        c.schedule()
        assertEquals(2, c.attempts)
        c.reset()
        assertEquals(0, c.attempts)
        assertEquals(null, s.pending)
        c.schedule()
        assertEquals(2_000L, s.delays.last())
    }

    @Test
    fun `fire triggers attempt when active`() {
        val s = FakeScheduler()
        var fired = 0
        val c = newController(s) { fired++ }
        c.start()
        c.schedule()
        s.pending?.invoke()
        assertEquals(1, fired)
    }

    @Test
    fun `stop prevents further attempts`() {
        val s = FakeScheduler()
        var fired = 0
        val c = newController(s) { fired++ }
        c.start()
        c.schedule()
        c.stop()
        assertFalse(c.active)
        assertEquals(null, s.pending)
        s.pending?.invoke()
        assertEquals(0, fired)
    }
}
