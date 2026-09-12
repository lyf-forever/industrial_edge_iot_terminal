# 工业边缘物联终端 — Android APP

本目录为智能终端项目的移动端 APP，**Android 原生 Kotlin** 实现，覆盖设计文档
`docs/smart_iot_terminal_guide.docx` 要求的移动端五大能力，并接入工程涉及的
全部通信链路：

| 链路 | 协议 | 状态 | APP 功能 |
| ---- | ---- | ---- | -------- |
| 云链路 | MQTT 3.1.1（Paho） | 固件已实现（cloud_bridge） | 数据监控、远程配置、OTA 触发、LCD 显示控制 |
| BLE 链路 | GATT（Nordic UART Service） | 契约约定，待 ESP32 GATT Server 固件 | 设备扫描、RSSI 实时曲线、透传收发 |
| 经典蓝牙链路 | SPP（RFCOMM） | 对应 GD32 BT24/HC05 透传模块 | 设备发现、双向透传 |
| LAN 链路 | TCP | 目标为 ESP32 Wi-Fi 透传服务（规划） | 双向透传 |

## 功能页一览（v0.7.0）

| 页 | 功能 |
| -- | ---- |
| 监控 | 实时展示温度/湿度/气体/CO₂/气压；五指标历史曲线（Spinner 切换）；**24h 小时聚合统计**（均温/均湿/气体峰值）；设备在线判定（心跳 60s 超时判离线，显示最后心跳与运行时长）；告警历史（最近 50 条，置顶展示，≥2 级通知推送，**支持通知开关/勿扰时段/一键清空**）；传感器数据落盘 SQLite（保留 30 天自动清理，可查询/清空/导出 CSV 最近 5000 条分享）；MQTT 收发日志（时间序、自动滚动）；调试发布面板（任意主题发布/订阅） |
| BLE | 扫描附近设备（按 RSSI 降序排序）、点击连接 GATT（**协商 MTU 185**）、RSSI 实时曲线（扫描态 + 连接态）、透传收发（HEX/ASCII、时间戳 + RX/TX 字节计数 + 自动滚动） |
| 透传 | 经典蓝牙 SPP 与 LAN TCP 双通道切换；SPP 设备发现/配对提示、已连接设备列表标记；SPP/TCP 断线自动重连（指数退避 2s→30s，连接中操作有提示，点击其他设备自动切换目标）；共用收发面板（HEX/ASCII、时间戳、计数、自动滚动） |
| 控制 | LED 开关、LCD 显示下发、状态查询、远程重启；OTA 固件升级触发；命令历史（时间戳 + 发送/QoS1 确认结果）；**命令模板**（收藏 JSON 命令，点击即发、长按删除，上限 10 条） |
| 设置 | MQTT（Broker/Client ID）、BLE 契约 UUID、TCP 目标地址配置；后台保活开关（前台服务）；全量配置备份/导入导出（JSON，键白名单校验，连接中导入/保存提示需重连生效） |

## 架构要点（v0.7.0）

- **共享 ViewModel（活动级）**：`AppViewModel`（AndroidViewModel）作为 MqttManager
  的唯一业务订阅者，集中持有曲线/日志/告警/命令历史/在线状态，**页面切换数据不丢失**；
  高等级告警通知不依赖页面前台状态；
- **MQTT 后台保活**：`MqttForegroundService` 前台服务（dataSync 类型），
  锁屏/退后台保持连接，常驻通知实时显示连接状态，可一键停止；
- **统一自愈重连**：`ReconnectController`（可注入调度器）统一 MQTT/SPP/TCP
  三链路指数退避（成功清零）+ 网络恢复立即重试，用户主动断开后停止；
- **数据治理**：传感器历史保留 30 天自动清理，CSV 导出分享，24h 聚合统计；
- **配置可迁移**：全部连接参数一键导出/导入 JSON（纯函数核心 + 键白名单校验）；
- **质量基线**：JVM 单元测试 32 例（字节编解码/退避/时间格式/CSV/JSON/勿扰窗口）
  随 CI 执行；lint 非策略性告警清零（详见"依赖与 SDK 版本策略"）；
- **UI**：深色模式（values-night）、启动画面（SplashScreen API）、
  图表取色随主题。

## 权限说明

| 权限 | 用途 |
| ---- | ---- |
| INTERNET / ACCESS_NETWORK_STATE | MQTT、TCP |
| BLUETOOTH_SCAN / BLUETOOTH_CONNECT（Android 12+） | BLE 扫描/GATT、SPP 发现 |
| BLUETOOTH / BLUETOOTH_ADMIN、ACCESS_FINE_LOCATION（≤Android 11） | 旧版本蓝牙扫描 |
| POST_NOTIFICATIONS（Android 13+） | 高等级告警通知栏推送 |
| FOREGROUND_SERVICE / FOREGROUND_SERVICE_DATA_SYNC | MQTT 后台保活前台服务 |

运行时权限在首次使用 BLE/透传页时按需请求；通知权限在 APP 启动时申请。

## 与固件的链路契约

### MQTT（对照 `ESP32S3/Service/Inc/mqtt_client_app.h` + `cloud_bridge.c`）

| 方向 | 主题 | 载荷示例 |
| ---- | ---- | -------- |
| 设备→云 | `ind_edge/terminal/sensors` | `{"temp":25.3,"humid":61.0,"gas":120,"co2":450,"press":1013}` |
| 设备→云 | `ind_edge/terminal/alarm` | `{"alarm_id":1,"level":2,"val":500}` |
| 设备→云 | `ind_edge/terminal/status` | `{"heartbeat":10,"uptime_s":3600}` |
| 云→设备 | `ind_edge/terminal/cmd` | `{"cmd":"led","id":1,"state":1}` 等 |

下行命令（固件 `on_mqtt_msg` 以 strstr 识别关键字，字段名不可改）：
`led`（id/state）、`display`（text，→ EVT_CTRL_DISPLAY 跨核到 GD32 LCD）、
`status`、`reboot`、`ota`（url）。

### BLE（契约约定，供 ESP32 GATT Server 固件对齐，默认 Nordic UART Service）

| 项 | UUID |
| -- | ---- |
| 服务 | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| 写特征（APP→设备） | `6E400002-…` |
| 通知特征（设备→APP） | `6E400003-…` |

三者均可在“设置”页修改；连接成功后自动请求 MTU 185。

### SPP / TCP

- SPP：标准 UUID `00001101-0000-1000-8000-00805F9B34FB`，对应 GD32 端
  BT24/HC05（9600bps 串口透传）与 ESP32 内置经典蓝牙；未配对设备需先在
  系统蓝牙中配对；
- TCP：默认 `192.168.4.1:8080`（可在设置页改），面向后续 ESP32 Wi-Fi 透传服务。

## 本地数据（SQLite）

传感器数据按 3s 节流落盘 `sensor_history.db`（表 `history`：时间戳/温度/湿度/
气体/CO₂/气压），保留 30 天，启动时自动清理过期记录。
监控页"查询历史"回看最近 50 条、"24h 统计"按小时聚合（均温/均湿/气体峰值）、
"清空历史"删除全部、"导出 CSV"生成最近 5000 条并经系统分享。

## 构建

1. Android Studio（Koala 及以上，自带 JDK 17）直接打开本目录同步即可；
2. 命令行（需 JDK 17 + Android SDK，配置 `local.properties` 指向 SDK）：

```bash
gradlew.bat assembleDebug        # Windows
# 产物: app/build/outputs/apk/debug/app-debug.apk
```

环境要求：Gradle 8.6（wrapper 自动下载）、AGP 8.4.2、Kotlin 1.9.24、
compileSdk 34 / minSdk 26 / targetSdk 34。
依赖：AppCompat、Fragment-KTX、Material、ConstraintLayout、Core-Splashscreen、
Eclipse Paho MQTT（1.2.5）。

CI（`.github/workflows/android.yml`）：push/PR 触发 assembleDebug +
testDebugUnitTest + lintDebug，产物上传 APK 与 lint 报告 artifact。

## 依赖与 SDK 版本策略

版本采用**钉子策略**（内部分发，不上架应用商店）：

- targetSdk 34、依赖版本在**维护窗口统一升级**，升级时执行真机回归
  （深色模式、状态栏/手势区、通知、蓝牙、前台服务）；
- lint 的 `OldTargetApi` 与 `GradleDependency` 为版本提示类告警，
  已按策略在 `app/build.gradle.kts` 中关闭，升级任务单独跟踪；
- 其余 lint 检查保持开启且错误终止构建；`HardcodedText`/`SetTextI18n`
  因单语种运维工具定位关闭（静态文案多为日志/动态消息）；
- 每次升级需通过 CI（build + 32 项单测 + lint）并更新本节版本号。

## 目录结构

```
Android_APP/
├── app/src/main/
│   ├── AndroidManifest.xml
│   ├── java/com/indedge/terminal/app/
│   │   ├── MainActivity.kt          # 底部导航（5 页）+ 全局设施初始化
│   │   ├── mqtt/MqttManager.kt      # MQTT 契约 + Paho 封装 + 自愈重连
│   │   ├── ble/BleManager.kt        # BLE 扫描 + GATT 透传 + RSSI + MTU
│   │   ├── bt/SppManager.kt         # 经典蓝牙 SPP 透传
│   │   ├── net/TcpManager.kt        # LAN TCP 透传
│   │   ├── db/SensorStore.kt        # SQLite 历史落盘 + 小时聚合
│   │   ├── service/MqttForegroundService.kt  # MQTT 后台保活前台服务
│   │   ├── util/                    # Prefs/ByteCodec/Notifier/AlarmPrefs/
│   │   │                            # Backoff/ReconnectController/TimeFmt/CsvExport/
│   │   │                            # ConfigBackup/ShareUtils
│   │   ├── view/                    # RssiChartView / LineChartView
│   │   └── ui/                      # AppViewModel + Status/Ble/PassThrough/Control/Settings
│   └── res/                         # 布局 / 主题（含 values-night）/ 图标 / 菜单
├── app/src/test/                    # JVM 单元测试（32 例）
├── gradle/wrapper/                  # Gradle 8.6 wrapper
└── README.md
```

## 后续路线（v0.8+，均需固件协同或维护窗口）

- 固件联动：BLE GATT Server（NUS）、TCP Server、OTA 进度/设备 ACK/版本上报
  （协议草案见 `docs/app_firmware_contract.docx`）；
- 维护窗口：targetSdk/依赖统一升级 + 真机回归；
- 产品化：release 签名流程、"关于"页；
- 可选：Espresso 冒烟测试（模拟器 CI）。
