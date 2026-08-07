/// @file      text_file.hpp
/// @brief     文本文件读写辅助
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_FILE_TEXT_FILE_HPP_
#define QTRADE_COMMON_FILE_TEXT_FILE_HPP_

#include <optional>
#include <string>

namespace qtrade::common {

/// @brief 读取文本文件全部内容
/// @param path 文件路径
/// @return 成功返回文件内容；打开失败返回 nullopt
[[nodiscard]] std::optional<std::string> ReadTextFile(const std::string& path);

}  // namespace qtrade::common

#endif  // QTRADE_COMMON_FILE_TEXT_FILE_HPP_
