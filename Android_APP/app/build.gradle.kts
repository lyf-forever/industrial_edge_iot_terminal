plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.indedge.terminal.app"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.indedge.terminal.app"
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "0.1.0"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }

    buildFeatures {
        viewBinding = true
    }

    lint {
        // 单语种工业运维工具：静态文案以日志/动态消息为主
        disable += setOf("HardcodedText", "SetTextI18n")
        // 版本钉子策略（内部分发，不上架应用商店）：
        // targetSdk 与依赖版本在维护窗口统一升级并做真机回归（流程见 README），
        // 此处固定关闭版本提示类告警，避免每次构建噪音；升级任务单独跟踪。
        disable += setOf("OldTargetApi", "GradleDependency")
        abortOnError = true
    }
}

dependencies {
    implementation("androidx.appcompat:appcompat:1.7.0")
    implementation("androidx.fragment:fragment-ktx:1.7.1")
    implementation("androidx.core:core-splashscreen:1.0.1")
    implementation("com.google.android.material:material:1.12.0")
    implementation("androidx.constraintlayout:constraintlayout:2.1.4")
    // MQTT 3.1.1 客户端（与 ESP32 esp-mqtt / OneNET 协议兼容）
    implementation("org.eclipse.paho:org.eclipse.paho.client.mqttv3:1.2.5")

    testImplementation("junit:junit:4.13.2")
    // JVM 单测用真实 org.json 实现（替代 android.jar 桩）
    testImplementation("org.json:json:20231013")
}
