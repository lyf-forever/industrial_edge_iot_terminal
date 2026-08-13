#ifndef __SYS_H_
#define __SYS_H_

#include <stdio.h>
#include <stdint.h>
#include "esp_log.h"

/* 结构体 */


/* 传感器使用 */
#define SensorUse    1     /* 0: 不使用传感器 1：使用传感器 */
/* 传感器详情 */
#define Mq2Use       1     /* 0: 不使用MQ2 1：使用MQ2 */

/* 板级外设 */
#define Ws2812Use    1     /* 0: 不使用WS2812灯带 1：使用WS2812灯带 */

/* 串口链路（与 GD32H7 端通信，复用 link_protocol） */
#define LinkUse      1     /* 0: 不使用串口链路 1：使用串口链路 */

/* 无线通信 */
#define WiFiUse      1     /* 0: 不使用Wi-Fi 1：使用Wi-Fi */
#define MqttUse      1     /* 0: 不使用MQTT 1：使用MQTT (依赖 WiFiUse) */

/* 云端桥接（link<->MQTT 数据路由，依赖 LinkUse 与 MqttUse） */
#define CloudUse     1     /* 0: 不使用云端桥接 1：使用云端桥接 */

/* 事件总线（系统级消息中枢，跨核透明发布订阅） */
#define EventBusUse 1     /* 0: 不使用事件总线 1：使用事件总线 */

/* 连接层次状态机（HSM，订阅通信状态事件驱动指示灯） */
#define HsmUse      1     /* 0: 不使用HSM 1：使用HSM */

/* ===================== 新架构扩展（software_architecture 文档 3.x） ====== */
/* 3.1 模块注册表与依赖（激活 initItem.depends_on 拓扑排序） */
#define ModRegUse   1
/* 3.6 内存池/对象池（PSRAM 定长块位图分配）
 * IRAM 裁剪：mempool_alloc/free 无任何调用者，关闭以减小固件体积 */
#define MemPoolUse  0
/* 3.8 软定时器轮（1/10/100ms 三层时间轮）
 * IRAM 裁剪：soft_timer_add/del 无调用者（系统节拍由 TTS 提供），关闭 */
#define SoftTimerUse 0
/* 3.3 通道抽象（channel_t 接口 + UART 实现 + SPI 占位） */
#define ChannelUse  1
/* 3.3b HS-SPI 真实通道驱动（依赖 ChannelUse；0 则用 SPI 占位 stub）
 * IRAM 裁剪：SPI 从机侧未完成联调（RX 无轮询、切换后回调不生效），
 * 关闭可移除 SPI master 驱动 ISR 的 IRAM 占用；联调完成后置 1 */
#define ChannelSpiUse 0
/* 3.7 命令分发器（命令表 + ACL） */
#define CmdUse      1
/* 3.10 通用 HSM 框架（泛化 conn_hsm） */
#define HsmFwUse    1
/* 3.9 结构化日志框架 */
#define LogUse      1
/* 3.5 时间触发调度器 TTS */
#define TtsUse      1
/* 3.4 Actor 模型（消息队列任务）
 * IRAM 裁剪：actor_post 无调用者（日志落盘 Actor 仅为演示），关闭 */
#define ActorUse    0
/* 3.2 主题化发布订阅（分层主题 + 通配）
 * IRAM 裁剪：topic_subscribe 无调用者，关闭 */
#define TopicUse    0
/* 3.12 安全凭证管理器（NVS） */
#define CredUse     1
/* 3.11 A/B 分区 OTA 状态机 */
#define OtaUse      1
/* 3.13 边缘 AI 推理管线（轻量异常检测） */
#define AiUse       1

/* API declare */
void delay_us(uint32_t us);

/* 功能开关宏组合编译期校验（架构 6.7 风险缓解） */
#include "config_verify.h"

#endif 