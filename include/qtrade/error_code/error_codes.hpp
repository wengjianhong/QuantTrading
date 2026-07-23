/// @file      error_codes.hpp
/// @brief     错误码定义
/// @details   定义所有错误码的枚举类型，采用 AA BBB CCC 八位整数编码方案
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ERROR_CODE_ERROR_CODES_HPP_
#define QTRADE_ERROR_CODE_ERROR_CODES_HPP_
#include <qtrade/error_code/code_segment.hpp>

#include <cstdint>
#include <string_view>

namespace qtrade {
using cpputils::error_code::MakeErrorCode;

/// 错误码：AABBBCCC 八位整数，AA=10（系统级），BBB=模块，CCC=具体错误；0 表示成功
enum class ErrorCode : int32_t {
  /// ============================ 通用错误码模块 ============================
  /// 成功
  kSuccess = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kCommon), 0),
  /// 通用错误
  kError = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kCommon), 1),
  //// 未初始化
  kNotInitialized = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kCommon), 2),
  //// 内部错误
  kInternalError = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kCommon), 3),
  //// 超时
  kTimeout = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kCommon), 4),
  //// 资源耗尽
  kResourceExhausted = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kCommon), 5),
  //// 未找到
  kNotFound = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kCommon), 6),
  /// 不支持
  kNotSupported = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kCommon), 7),

  /// ============================ 系统错误码模块 ============================
  /// 操作系统或运行时错误
  kSystemError = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kSystemError), 0),
  /// 参数无效
  kInvalidArgument = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kSystemError), 1),
  /// 当前状态不允许执行操作
  kInvalidState = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kSystemError), 2),
  /// 权限不足
  kPermissionDenied = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kSystemError), 3),
  /// 操作被取消
  kCancelled = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kSystemError), 4),

  /// ============================ 网络错误码模块 ============================
  /// 通用网络错误
  kNetworkError = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kNetworkError), 0),
  /// 网络连接失败
  kNetworkConnectError = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kNetworkError), 1),
  /// 网络连接已断开
  kNetworkDisconnected = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kNetworkError), 2),
  /// 网络发送失败
  kNetworkSendError = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kNetworkError), 3),
  /// 网络接收失败
  kNetworkReceiveError = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kNetworkError), 4),

  /// ============================ SQL错误码模块 ============================
  /// 通用SQL错误
  kSqlError = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kSqlError), 0),
  /// 连接错误
  kConnectionError = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kSqlError), 1),
  /// SQL 执行失败
  kSqlExecuteError = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kSqlError), 2),
  /// SQL 查询失败
  kSqlQueryError = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kSqlError), 3),
  /// 数据库事务开启失败
  kSqlBeginTransactionError = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kSqlError), 4),
  /// 数据库事务提交失败
  kSqlCommitTransactionError = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kSqlError), 5),
  /// 数据库事务回滚失败
  kSqlRollbackTransactionError = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kSqlError), 6),
  /// 数据库记录不存在
  kSqlNotFound = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kSqlError), 7),
  /// 数据库唯一约束冲突
  kSqlConflict = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kSqlError), 8),
  /// 数据库提交失败
  kCommitError = MakeErrorCode(static_cast<uint64_t>(ModuleNumber::kSqlError), 9),
};

/// @brief 获取错误码描述
/// @param code 错误码
/// @return 错误码对应的字符串描述；未定义的错误码返回 "UnknownError"
[[nodiscard]] const std::string_view GetErrorCodeMessage(ErrorCode code);

}  // namespace qtrade

#endif  // QTRADE_ERROR_CODE_ERROR_CODES_HPP_
