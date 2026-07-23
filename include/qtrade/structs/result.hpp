/// @file      result.hpp
/// @brief     框架通用返回结果
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_STRUCTS_RESULT_HPP_
#define QTRADE_STRUCTS_RESULT_HPP_

#include <qtrade/error_code/error_codes.hpp>

#include <optional>
#include <string>
#include <vector>

namespace qtrade {

template <typename T>
struct Result {
  /// 错误码
  ErrorCode error_code = ErrorCode::kSuccess;
  /// 错误信息
  std::string error_message;
  /// 返回数据
  std::optional<T> data = std::nullopt;
  /// 错误信息参数
  std::optional<std::vector<std::string>> error_message_args = std::nullopt;
};

template <>
struct Result<void> {
  /// 错误码
  ErrorCode error_code = ErrorCode::kSuccess;
  /// 错误信息
  std::string error_message;
  /// 错误信息参数
  std::optional<std::vector<std::string>> error_message_args = std::nullopt;
};

}  // namespace qtrade

#endif  // QTRADE_STRUCTS_RESULT_HPP_
