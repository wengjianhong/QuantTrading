/// @file      code_message.cpp
/// @brief     错误码描述实现
/// @details   实现 GetErrorCodeMessage 等错误码描述查询逻辑
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include <qtrade/error_code/code_message.hpp>

namespace qtrade {

const std::string_view GetErrorCodeMessage(ErrorCode code) {
  auto it = kErrorCodeMessages.find(code);
  if (it != kErrorCodeMessages.end()) {
    return it->second;
  }
  return "UnknownError";
}

}  // namespace qtrade
