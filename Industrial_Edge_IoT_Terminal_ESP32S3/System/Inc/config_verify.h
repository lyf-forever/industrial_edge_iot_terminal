#ifndef __CONFIG_VERIFY_H_
#define __CONFIG_VERIFY_H_

/**
 * @file config_verify.h
 * @brief 功能开关宏组合编译期校验（文档 6.7 风险缓解）
 *
 * 对 sys.h 中的 XxxUse 宏做依赖一致性校验，非法组合编译时报错，
 * 避免"开关裁剪导致链接失败/功能缺失"的隐性错误。
 * 在 sys.h 末尾 #include 本文件。
 */

/* ---- 依赖校验 ---- */
#if MqttUse && !WiFiUse
#error "MqttUse=1 但 WiFiUse=0：MQTT 依赖 Wi-Fi 连接，请同时启用 WiFiUse"
#endif

#if CloudUse && !MqttUse
#error "CloudUse=1 但 MqttUse=0：云端桥接依赖 MQTT，请同时启用 MqttUse"
#endif

#if HsmUse && !EventBusUse
#error "HsmUse=1 但 EventBusUse=0：连接 HSM 依赖事件总线，请同时启用 EventBusUse"
#endif

#if ChannelUse && !LinkUse
#error "ChannelUse=1 但 LinkUse=0：通道抽象承载串口链路，请同时启用 LinkUse"
#endif

#if ChannelSpiUse && !ChannelUse
#error "ChannelSpiUse=1 但 ChannelUse=0：HS-SPI 通道依赖通道抽象，请同时启用 ChannelUse"
#endif

#if OtaUse && !HsmFwUse
#error "OtaUse=1 但 HsmFwUse=0：OTA 状态机依赖通用 HSM 框架，请同时启用 HsmFwUse"
#endif

#if AiUse && !EventBusUse
#error "AiUse=1 但 EventBusUse=0：边缘 AI 管线依赖事件总线，请同时启用 EventBusUse"
#endif

#if TopicUse && !EventBusUse
#error "TopicUse=1 但 EventBusUse=0：主题订阅依赖事件总线，请同时启用 EventBusUse"
#endif

#if CredUse && !WiFiUse && !MqttUse
#error "CredUse=1 但 WiFiUse/MqttUse 均关闭：凭证管理器当前仅服务 Wi-Fi/MQTT，请启用其一或关闭 CredUse"
#endif

#if ActorUse && !EventBusUse
/* Actor 可独立使用（不强制依赖事件总线），仅提示 */
#endif

#if ModRegUse && !EventBusUse
/* mod_registry 是纯调度增强，不依赖事件总线 */
#endif

#endif /* __CONFIG_VERIFY_H_ */