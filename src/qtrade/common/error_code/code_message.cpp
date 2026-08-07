/// @file      code_message.cpp
/// @brief     错误码描述实现
/// @details   实现 GetErrorCodeMessage 等错误码描述查询逻辑
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include <qtrade/error_code/error_codes.hpp>

#include <unordered_map>

namespace qtrade {
namespace {

/// @brief 错误码描述
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

}  // namespace

const std::string_view GetErrorCodeMessage(ErrorCode code) {
  auto it = kErrorCodeMessages.find(code);
  if (it != kErrorCodeMessages.end()) {
    return it->second;
  }
  return "UnknownError";
}

}  // namespace qtrade
