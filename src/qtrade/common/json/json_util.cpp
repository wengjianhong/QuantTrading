/// @file      json_util.cpp
/// @brief     JSON 字符串解析与文件加载辅助实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/json/json_util.hpp"

#include "qtrade/common/file/text_file.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::common {

std::optional<nlohmann::json> ParseJsonString(const std::string& json) {
  try {
    return nlohmann::json::parse(json);
  } catch (const nlohmann::json::exception& ex) {
    spdlog::warn("[Json] invalid JSON string: {}", ex.what());
    return std::nullopt;
  }
}

std::optional<nlohmann::json> LoadJsonFile(const std::string& json_path) {
  const auto text = ReadTextFile(json_path);
  if (!text.has_value()) {
    return std::nullopt;
  }
  return ParseJsonString(*text);
}

}  // namespace qtrade::common
