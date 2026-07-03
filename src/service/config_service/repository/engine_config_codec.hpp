/// @file      engine_config_codec.hpp
/// @brief     EngineConfig 与 JSON 互转（DB 持久化与 proto 模型对齐）
/// @author    wengjianhong
/// @date      2026-07-02
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ENGINE_CONFIG_CODEC_HPP_
#define QTRADE_SERVICE_ENGINE_CONFIG_CODEC_HPP_

#include <qtrade/config/v1/config.pb.h>

#include <string>

namespace qtrade::service {

/// @brief 将 EngineConfig 序列化为 JSON 字符串（Proto JSON 映射）
/// @param config 引擎业务配置
/// @param out 输出 JSON
/// @return true 表示成功
[[nodiscard]] bool EngineConfigToJson(const qtrade::config::v1::EngineConfig& config, std::string& out);

/// @brief 从 JSON 字符串解析 EngineConfig
/// @param json JSON 文本
/// @param config 输出引擎业务配置
/// @return true 表示成功
[[nodiscard]] bool EngineConfigFromJson(const std::string& json, qtrade::config::v1::EngineConfig& config);

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ENGINE_CONFIG_CODEC_HPP_
