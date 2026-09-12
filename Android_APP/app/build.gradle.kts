import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.Properties

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

// 发布签名：读取 Android_APP/keystore.properties（本地生成，不入库）
// 模板见 keystore.properties.template；缺失时 release 走未签名产物
val keystoreProps = Properties().apply {
    val f = rootProject.file("keystore.properties")
    if (f.exists()) f.inputStream().use { load(it) }
}

android {
    namespace = "com.indedge.terminal.app"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.indedge.terminal.app"
        minSdk = 26
        targetSdk = 34
        versionCode = 7
        versionName = "0.7.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        // 构建时间（关于页展示）
        buildConfigField(
            "String",
            "BUILD_TIME",
            "\"${SimpleDateFormat("yyyy-MM-dd HH:mm", Locale.US).format(Date())}\""
        )
    }

    signingConfigs {
        if (keystoreProps.isNotEmpty()) {
            create("release") {
                storeFile = file(keystoreProps.getProperty("storeFile"))
                storePassword = keystoreProps.getProperty("storePassword")
                keyAlias = keystoreProps.getProperty("keyAlias")
                keyPassword = keystoreProps.getProperty("keyPassword")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            if (keystoreProps.isNotEmpty()) {
                signingConfig = signingConfigs.getByName("release")
            }
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
        buildConfig = true
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

    // 仪器化冒烟测试（模拟器运行）
    androidTestImplementation("androidx.test.ext:junit:1.2.1")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.6.1")
}
