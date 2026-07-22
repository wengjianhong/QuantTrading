/// @file      code_message.hpp
/// @brief     错误码描述映射表
/// @details   提供错误码到人类可读描述的查找与映射
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ERROR_CODE_CODE_MESSAGE_HPP_
#define QTRADE_ERROR_CODE_CODE_MESSAGE_HPP_
#include <qtrade/error_code/error_codes.hpp>

#include <cstdint>
#include <string_view>
#include <unordered_map>

namespace qtrade {

enum class ErrorCode : int32_t;

/// @brief   获取错误码描述
/// @param code 错误码
/// @return 错误码对应的字符串描述
const std::string_view GetErrorCodeMessage(ErrorCode code);

/// @brief   错误码描述
const std::unordered_map<ErrorCode, const std::string_view> kErrorCodeMessages = {
  /// 公共模块
  {ErrorCode::kSuccess, "Success"},
  {ErrorCode::kError, "Error"},
  {ErrorCode::kNotInitialized, "Not Initialized"},
  {ErrorCode::kInternalError, "Internal Error"},
  {ErrorCode::kTimeout, "Timeout"},
  {ErrorCode::kResourceExhausted, "Resource Exhausted"},
  {ErrorCode::kNotFound, "Not Found"},
  {ErrorCode::kNotSupported, "Not Supported"},

  /// 系统错误码段
  {ErrorCode::kSystemError, "System Error"},
  {ErrorCode::kInvalidArgument, "Invalid Argument"},
  {ErrorCode::kInvalidState, "Invalid State"},
  {ErrorCode::kPermissionDenied, "Permission Denied"},
  {ErrorCode::kCancelled, "Cancelled"},

  /// 网络错误码段
  {ErrorCode::kNetworkError, "Network Error"},
  {ErrorCode::kNetworkConnectError, "Network Connect Error"},
  {ErrorCode::kNetworkDisconnected, "Network Disconnected"},
  {ErrorCode::kNetworkSendError, "Network Send Error"},
  {ErrorCode::kNetworkReceiveError, "Network Receive Error"},

  /// Sql错误码段
  {ErrorCode::kConnectionError, "Connection Error"},
  {ErrorCode::kSqlExecuteError, "SqlExecute Error"},
  {ErrorCode::kSqlQueryError, "Sql Query Error"},
  {ErrorCode::kSqlBeginTransactionError, "Sql BeginTransaction Error"},
  {ErrorCode::kSqlCommitTransactionError, "Sql CommitTransaction Error"},
  {ErrorCode::kSqlRollbackTransactionError, "Sql RollbackTransaction Error"},
  {ErrorCode::kSqlNotFound, "Sql Not Found"},
  {ErrorCode::kSqlConflict, "Sql Conflict"},
  {ErrorCode::kCommitError, "Commit Error"},
};

}  // namespace qtrade

#endif  // QTRADE_ERROR_CODE_CODE_MESSAGE_HPP_