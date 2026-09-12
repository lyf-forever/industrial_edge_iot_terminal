# 双端固件联调快速指南（Bring-up Guide）

适用：ESP32S3（ESP-IDF v5.5.4）+ GD32H7（arm-none-eabi）+ Android APP v0.7.0+
本文档配合 `Android_APP/DEVICE_TEST_CHECKLIST.md` 使用：先按本文完成链路打通，再走完整清单。

---

## 0. 前置与重要提醒

- 激活 ESP-IDF（EIM 安装示例）：
  `. C:\Espressif\tools\Microsoft.v5.5.4.PowerShell_profile.ps1`
- GD32 编译（仓库自带工具链文件）：
  `cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake && cmake --build build`
- ⚠️ **协议兼容性**：本版本修复了链路协议两处历史缺陷（**转义范围收敛为仅 data 区**、
  **CRC 线上大端（高字节先发）**）。**ESP32 与 GD32 必须同时烧录本版**，
  与旧固件混用将导致几乎全部帧校验失败。

## 1. 烧录

```bash
# ESP32（在 Industrial_Edge_IoT_Terminal_ESP32S3 目录）
idf.py -p COMx flash monitor

# GD32：烧录 build/Industrial_Edge_IoT_Terminal_GD32H7.hex（J-Link / ST-Link / DAP-Link）
```

## 2. ESP32 启动日志核对（串口 monitor）

应依次出现（关键行）：

```
mod_reg: [stage ..] ...s wifi_mgr / mqtt_app / cloud_br / net_bridge ...
net_bridge: BLE GATT start requested (NUS)
net_bridge: TCP passthrough server on port 8080
net_bridge: BLE advertising as "ind_edge_esp32s3"
wifi_manager: got ip: 192.168.x.x
MQTT_EVENT_CONNECTED
event_ipc: event ipc bridge init ok
```

| 故障现象 | 排查 |
| ---- | ---- |
| `bt controller init failed` | IRAM 不足或 BT 配置异常；确认 sdkconfig `CONFIG_BT_ENABLED/BLUEDROID/BLE/42_FEATURES` 均为 y |
| 无 `BLE advertising` | 确认 `CONFIG_BT_BLE_42_FEATURES_SUPPORTED=y`（传统广播 API 必需） |
| 无 `TCP passthrough server` | `TcpSrvUse=0` 或 8080 被占用 |
| 编译报 `component 'mdns' not found` | 保持 `MdnsUse=0`（默认）；启用 mDNS 参见第 8 节 |

## 3. 云链路（APP ↔ 云 ↔ 设备）

1. APP 设置页：Broker 默认 `tcp://broker.emqx.io:1883`，点"连接"；
2. 监控页：收到 sensors 数据；心跳行显示 `最后心跳 xx · 运行 xx · FW 0.5.0-esp`（版本字段来自 status 的 `ver`）；
3. 控制页：LED 开/关 → 命令历史出现 `→ {"cmd":"led"...}`、`  已确认`、`  回执[led] 设备已执行`（ack 主题）；
4. 状态查询 / 远程重启各自验证；重启后 30s 内自动重连。

## 4. BLE 透传验证（NUS）

1. APP BLE 页扫描 → 连接 `ind_edge_esp32s3`；
2. ESP 日志依次：`BLE client connected` → `NUS ready (rx=.. tx=.. cccd=..)` →
   APP 使能通知后 `BLE notify enabled`；`BLE MTU negotiated: 185`；
3. **接收方向自检**：APP 接收区（HEX）每 5 秒应出现一帧——ESP 每 5s 发布心跳事件，
   经跨核链路 TX 时镜像广播（约 21 字节，以 `AA 55` 开头）；
4. **发送方向验证**（需 GD32 在线）：HEX 模式发送 LED 点亮帧：

```
AA 55 01 10 0C 01 01 00 00 00 00 00 00 00 00 00 00 9C 78 0D 0A
```

   → GD32 端 LED1 点亮（帧经 link 解析 → EVT_CTRL_LED 跨核转发至 GD32 执行）。

## 5. LAN TCP 透传验证

1. ESP 日志取 Wi-Fi IP（或路由器查看），APP 透传页选 LAN TCP 填 `IP:8080` → 连接；
2. ESP 日志：`TCP client fd=.. connected`；
3. 同一示例帧发送 → 同样点亮 LED；每 5s 收到心跳镜像帧；
4. 命令行快速验证（PC）：`telnet <IP> 8080` 或
   `python -c "import socket;s=socket.create_connection(('<IP>',8080));print(s.recv(64).hex())"`。

## 6. OTA 闭环验证（A/B + 回滚）

1. 在 ESP32 工程 `build/` 目录启动简易 HTTP 服务：`python -m http.server 8000`；
2. APP 控制页 OTA 输入：`http://<PC_IP>:8000/Industrial_Edge_IoT_Terminal_ESP32S3.bin`
   （现场生产用 HTTPS + 带证书校验的地址）；
3. 观察：
   - APP OTA 进度条按 `…/ota` 主题状态推进：1 下载中(n%) → 2 校验切换 → 3 完成；
   - ESP 日志下载字节数递增；完成后设备重启；
   - 重启后监控页 `FW` 版本字段更新；
4. 失败演练：随意断开 HTTP 服务 → state=4（detail=download error），设备保持原固件运行。

## 7. 协议要点（v0.7.1 修复后，两端一致）

- 帧格式：`AA 55 | addr cmd len | data(仅此区转义) | crc_hi crc_lo | 0D 0A`
- CRC：CRC-16/Modbus（init 0xFFFF，poly 0xA001），覆盖 `addr+cmd+len+data`（未转义原值），
  **线上大端（高字节先发）**；
- 转义表（仅 data 区）：`AA→CC 01`、`55→CC 02`、`0D→CC 03`、`0A→CC 04`、`CC→CC CC`；
- 常用示例帧：

| 用途 | 帧（HEX） |
| ---- | ---- |
| LED1 点亮 | `AA 55 01 10 0C 01 01 00 00 00 00 00 00 00 00 00 00 9C 78 0D 0A` |
| 状态查询（空载荷） | `AA 55 01 03 00 F0 20 0D 0A` |
| 心跳（空载荷） | `AA 55 01 00 00 00 20 0D 0A` |
| 转义自测（载荷含全部转义字节） | `AA 55 01 11 0C CC 01 CC 02 CC 03 CC 04 CC CC 00 00 00 00 00 00 00 29 E9 0D 0A` |

## 8. mDNS（可选）

```bash
idf.py add-dependency "espressif/mdns"     # v5.5 起 mdns 为组件管理器依赖
# 并在 main/CMakeLists.txt PRIV_REQUIRES 追加 mdns，Sys.h 将 MdnsUse 置 1
```

启用后局域网可解析 `ind-edge.local`，服务类型 `_ind_edge._tcp`（端口 8080）。

## 9. 常见问题速查

| 现象 | 处理 |
| ---- | ---- |
| 双端互相收不到帧 | 确认两端均为本版本（协议修复后不兼容旧版）；确认波特率/接线（UART 主链路） |
| BLE 连接后无镜像帧 | 检查 APP 是否已使能通知（TX 特征 CCCD）；确认 GD32 在线（心跳由 ESP 产生，不依赖 GD32） |
| TCP 连上即断 | 检查 PC/手机与 ESP 是否同网段；8080 是否被防火墙拦截 |
| OTA 提示失败 | URL 可达性（先在手机浏览器打开该 URL）；固件大小与分区余量（当前余 58%） |
| 排查串口占满 | `idf.py monitor` 用 `Ctrl+]` 退出；烧录占用 COM 口时先关 monitor |
