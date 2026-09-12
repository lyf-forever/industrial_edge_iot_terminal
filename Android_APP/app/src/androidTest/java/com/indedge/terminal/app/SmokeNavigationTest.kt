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
 * 仪器化冒烟测试：五个功能页导航切换 + 关键控件可见性。
 * 在模拟器（CI smoke job）或真机上运行：./gradlew connectedDebugAndroidTest
 */
@RunWith(AndroidJUnit4::class)
class SmokeNavigationTest {

    @Test
    fun navigateAllPages() {
        ActivityScenario.launch(MainActivity::class.java).use {
            // 监控页（默认页）
            onView(withId(R.id.spMetric)).check(matches(isDisplayed()))

            // BLE 页
            onView(withId(R.id.nav_ble)).perform(click())
            onView(withId(R.id.btnScan)).check(matches(isDisplayed()))

            // 透传页
            onView(withId(R.id.nav_passthrough)).perform(click())
            onView(withId(R.id.etTcpHost)).check(matches(isDisplayed()))

            // 控制页
            onView(withId(R.id.nav_control)).perform(click())
            onView(withId(R.id.btnQueryStatus)).check(matches(isDisplayed()))

            // 设置页
            onView(withId(R.id.nav_settings)).perform(click())
            onView(withId(R.id.btnToggleConnect)).check(matches(isDisplayed()))

            // 返回监控页
            onView(withId(R.id.nav_status)).perform(click())
            onView(withId(R.id.tvTemp)).check(matches(isDisplayed()))
        }
    }
}
