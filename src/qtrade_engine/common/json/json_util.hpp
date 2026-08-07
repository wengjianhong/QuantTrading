/// @file      json_util.hpp
/// @brief     JSON 字符串解析与文件加载辅助
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_JSON_JSON_UTIL_HPP_
#define QTRADE_COMMON_JSON_JSON_UTIL_HPP_

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace qtrade::common {

/// @brief 将 JSON 字符串解析为根对象
/// @param json JSON 文本
/// @return 成功返回根对象；非法返回 nullopt
[[nodiscard]] std::optional<nlohmann::json> ParseJsonString(const std::string& json);

/// @brief 读取并解析 JSON 文件
/// @param json_path 文件路径
/// @return 成功返回根对象；打开失败或 JSON 非法返回 nullopt
[[nodiscard]] std::optional<nlohmann::json> LoadJsonFile(const std::string& json_path);

}  // namespace qtrade::common

#endif  // QTRADE_COMMON_JSON_JSON_UTIL_HPP_
