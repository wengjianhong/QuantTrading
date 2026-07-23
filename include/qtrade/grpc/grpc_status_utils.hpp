/// @file      grpc_status_utils.hpp
/// @brief     ErrorCode 与 grpc::Status 互转
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_FRAMEWORK_GRPC_GRPC_STATUS_UTILS_HPP_
#define QTRADE_FRAMEWORK_GRPC_GRPC_STATUS_UTILS_HPP_

#include <qtrade/error_code/error_codes.hpp>

#include <grpcpp/grpcpp.h>

namespace qtrade::framework::grpc {

/// @brief 将业务 ErrorCode 映射为 gRPC Status
/// @param code 业务错误码
/// @return 对应的 grpc::Status（kInternalError 映射为 INVALID_ARGUMENT，与历史实现保持一致）
[[nodiscard]] inline ::grpc::Status ToGrpcStatus(ErrorCode code) {
  switch (code) {
    case ErrorCode::kSuccess:
      return ::grpc::Status::OK;
    case ErrorCode::kNotFound:
      return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "not found");
    case ErrorCode::kInternalError:
      return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "invalid argument");
    default:
      return ::grpc::Status(::grpc::StatusCode::INTERNAL, "internal error");
  }
}

}  // namespace qtrade::framework::grpc

#endif  // QTRADE_FRAMEWORK_GRPC_GRPC_STATUS_UTILS_HPP_
