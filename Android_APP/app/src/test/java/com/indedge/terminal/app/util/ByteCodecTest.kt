package com.indedge.terminal.app.util

import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class ByteCodecTest {

    @Test
    fun `ascii encode`() {
        assertArrayEquals(
            "AB".toByteArray(Charsets.UTF_8),
            ByteCodec.encodeSend("AB", hex = false)
        )
    }

    @Test
    fun `hex encode with spaces ignored`() {
        assertArrayEquals(byteArrayOf(0xAB.toByte(), 0xCD.toByte()), ByteCodec.encodeSend("AB CD", hex = true))
    }

    @Test
    fun `invalid hex returns empty`() {
        assertTrue(ByteCodec.encodeSend("XYZ", hex = true).isEmpty())
    }

    @Test
    fun `odd length hex returns empty`() {
        assertTrue(ByteCodec.encodeSend("ABC", hex = true).isEmpty())
    }

    @Test
    fun `ascii decode`() {
        assertEquals("hi", ByteCodec.decodeShow("hi".toByteArray(Charsets.UTF_8), hex = false))
    }

    @Test
    fun `hex decode upper`() {
        assertEquals("FF 01", ByteCodec.decodeShow(byteArrayOf(0xFF.toByte(), 0x01), hex = true))
    }
}
