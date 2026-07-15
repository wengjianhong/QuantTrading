/// @file      text_file.cpp
/// @brief     文本文件读写辅助实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/file/text_file.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <sstream>

namespace qtrade::common {

std::optional<std::string> ReadTextFile(const std::string& path) {
  std::ifstream ifs(path);
  if (!ifs.is_open()) {
    spdlog::warn("[File] cannot open {}", path);
    return std::nullopt;
  }
  std::ostringstream oss;
  oss << ifs.rdbuf();
  return oss.str();
}

}  // namespace qtrade::common
