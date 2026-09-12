package com.indedge.terminal.app

import androidx.test.core.app.ActivityScenario
import androidx.test.espresso.Espresso.onView
import androidx.test.espresso.action.ViewActions.click
import androidx.test.espresso.assertion.ViewAssertions.matches
import androidx.test.espresso.matcher.ViewMatchers.isDisplayed
import androidx.test.espresso.matcher.ViewMatchers.withId
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Test
import org.junit.runner.RunWith

/**
 * 仪器化冒烟测试：五个功能页导航切换 + 首屏关键控件可见性。
 * 在模拟器（CI smoke job）或真机上运行：./gradlew connectedDebugAndroidTest
 *
 * 注意：断言统一选各页首屏控件（页面为滚动布局，深处控件不在首屏）。
 */
@RunWith(AndroidJUnit4::class)
class SmokeNavigationTest {

    @Test
    fun navigateAllPages() {
        ActivityScenario.launch(MainActivity::class.java).use {
            // 监控页（默认页）：连接状态行在首屏
            onView(withId(R.id.tvConnState)).check(matches(isDisplayed()))

            // BLE 页：扫描按钮在首屏
            onView(withId(R.id.nav_ble)).perform(click())
            onView(withId(R.id.btnScan)).check(matches(isDisplayed()))

            // 透传页：通道切换组在首屏
            onView(withId(R.id.nav_passthrough)).perform(click())
            onView(withId(R.id.channelGroup)).check(matches(isDisplayed()))

            // 控制页：LED 控制按钮在首屏
            onView(withId(R.id.nav_control)).perform(click())
            onView(withId(R.id.btnLed1On)).check(matches(isDisplayed()))

            // 设置页：Broker 输入框在首屏
            onView(withId(R.id.nav_settings)).perform(click())
            onView(withId(R.id.etBrokerUri)).check(matches(isDisplayed()))

            // 返回监控页
            onView(withId(R.id.nav_status)).perform(click())
            onView(withId(R.id.tvDeviceOnline)).check(matches(isDisplayed()))
        }
    }
}
