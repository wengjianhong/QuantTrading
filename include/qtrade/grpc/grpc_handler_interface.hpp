/// @file      grpc_handler_interface.hpp
/// @brief     gRPC Unary 接口处理器模板基类（参考 ugos_serv ApiHandlerInterface）
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_FRAMEWORK_GRPC_GRPC_HANDLER_INTERFACE_HPP_
#define QTRADE_FRAMEWORK_GRPC_GRPC_HANDLER_INTERFACE_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/structs/result.hpp>

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include <optional>
#include <string>
#include <utility>

namespace qtrade::framework::grpc {

/// ===========================================================================
/// ========================= 模板基类: 统一接口处理器 =========================
/// ===========================================================================
/// RequestProto:  请求结构体类型
/// ResponseProto: 返回业务数据类型
/// ServerData:    业务层数据结构体（框架内部流转使用）
template <typename RequestProto, typename ResponseProto, typename ServerData>
class GrpcHandlerInterface {
  /// @brief 方法名称（用于日志与后续权限规则匹配）
  const std::string method_name_;

 public:
  explicit GrpcHandlerInterface(std::string method_name) : method_name_(std::move(method_name)) {}
  virtual ~GrpcHandlerInterface() = default;

  /// 禁用拷贝/移动，避免多态对象误拷贝
  GrpcHandlerInterface(GrpcHandlerInterface&&) = delete;
  GrpcHandlerInterface(const GrpcHandlerInterface&) = delete;
  GrpcHandlerInterface& operator=(GrpcHandlerInterface&&) = delete;
  GrpcHandlerInterface& operator=(const GrpcHandlerInterface&) = delete;

 public:
  /// ==========================================================================
  /// ======================= 统一入口: 实现接口的固定流程 =======================
  /// ==========================================================================
  /// @brief 默认执行流程
  /// @param context  gRPC 请求的上下文指针
  /// @param request  gRPC 的请求结构体指针
  /// @param response gRPC 的响应结构体指针
  /// @return 返回结果
  Result<void> Run(::grpc::ServerContext* context, const RequestProto* request, ResponseProto* response);

 protected:
  /// ==========================================================================
  /// ========================= 可选覆盖的虚接口 ================================
  /// ==========================================================================
  /// @brief 检查接口请求权限
  /// @param context  gRPC 请求的上下文指针
  /// @param request  gRPC 的请求结构体指针
  /// @return 执行结果
  virtual Result<void> CheckPermission(::grpc::ServerContext* context, const RequestProto* request);

  /// ==========================================================================
  /// ========================= 子类必须实现的纯虚接口 ==========================
  /// ==========================================================================
  /// @brief 将 gRPC 请求的 proto 结构体转为业务层数据结构体
  /// @param context  gRPC 请求的上下文指针
  /// @param request  gRPC 的请求结构体指针
  /// @return 业务层数据结构体
  virtual Result<ServerData> ConvertToServerData(::grpc::ServerContext* context, const RequestProto* request) = 0;

  /// @brief 校验参数合法性（非空、格式、长度、枚举值等）
  /// @param server_data 业务层数据结构体
  /// @return 执行结果
  virtual Result<void> ValidateParams(ServerData& server_data) = 0;

  /// @brief 检查前置条件（如：账户已存在、资源配额等）
  /// @param server_data 业务层数据结构体
  /// @return 执行结果
  virtual Result<void> CheckPreconditions(ServerData& server_data) = 0;

  /// @brief 执行核心业务逻辑（DB 操作、业务计算、调用底层服务）
  /// @note 失败时自动回滚
  /// @param server_data 业务层数据结构体
  /// @return 执行结果
  virtual Result<void> ExecuteBusiness(ServerData& server_data) = 0;

  /// @brief 校验操作是否真正生效（如：检查 DB 数据、底层服务状态）
  /// @note 失败时自动回滚
  /// @param server_data 业务层数据结构体
  /// @return 执行结果
  virtual Result<void> VerifyExecutionEffective(ServerData& server_data) = 0;

  /// @brief 失败回滚（事务回滚、数据恢复、资源释放）
  /// @param server_data 业务层数据结构体
  virtual void Rollback(ServerData& server_data) = 0;

  /// @brief 通知其他服务（如：日志服务、审计服务等外部服务）
  /// @note 失败时不回滚
  /// @param server_data 业务层数据结构体
  /// @return 执行结果
  virtual Result<void> NotifyService(ServerData& server_data) = 0;

  /// @brief 构造最终返回结果（填充 data、格式化返回值）
  /// @note 失败时自动回滚
  /// @param server_data 业务层数据结构体
  /// @param response    gRPC 响应结构体指针
  /// @return 执行结果
  virtual Result<void> BuildResponse(ServerData& server_data, ResponseProto* response) = 0;
};

/// ===========================================================================
/// =========================== 默认的权限校验实现 =============================
/// ===========================================================================
template <typename RequestProto, typename ResponseProto, typename ServerData>
Result<void> GrpcHandlerInterface<RequestProto, ResponseProto, ServerData>::CheckPermission(
  ::grpc::ServerContext* context, const RequestProto* request) {
  (void)context;
  (void)request;
  /// 当前 qtrade 尚未接入权限中心，默认放行；后续可在此对接 GrpcPermissionMgr。
  return Result<void>{ErrorCode::kSuccess, "success"};
}

/// ===========================================================================
/// =========================== Run 方法实现固定流程 ============================
/// ===========================================================================
template <typename RequestProto, typename ResponseProto, typename ServerData>
Result<void> GrpcHandlerInterface<RequestProto, ResponseProto, ServerData>::Run(::grpc::ServerContext* context,
                                                                                const RequestProto* request,
                                                                                ResponseProto* response) {
  /// 前置检查: 空指针检查
  if (context == nullptr) {
    spdlog::error("GrpcHandlerInterface context is nullptr: method={}", method_name_);
    return Result<void>{ErrorCode::kInternalError, "context is nullptr"};
  }
  if (request == nullptr) {
    spdlog::error("GrpcHandlerInterface request is nullptr: method={}", method_name_);
    return Result<void>{ErrorCode::kInternalError, "request is nullptr"};
  }
  if (response == nullptr) {
    spdlog::error("GrpcHandlerInterface response is nullptr: method={}", method_name_);
    return Result<void>{ErrorCode::kInternalError, "response is nullptr"};
  }

  std::optional<ServerData> server_data;
  bool rollback_on_exception = false;

  try {
    /// 前置检查: 检查接口请求权限
    const auto perm_result = CheckPermission(context, request);
    if (perm_result.error_code != ErrorCode::kSuccess) {
      spdlog::warn(
        "GrpcHandlerInterface CheckPermission failed: method={}, {}", method_name_, perm_result.error_message);
      return perm_result;
    }

    /// 步骤1: 将外部 RequestProto 转换为内部 ServerData 结构体
    Result<ServerData> convert_result = ConvertToServerData(context, request);
    if (convert_result.error_code != ErrorCode::kSuccess) {
      spdlog::warn(
        "GrpcHandlerInterface ConvertToServerData failed: method={}, {}", method_name_, convert_result.error_message);
      return {convert_result.error_code,
              std::move(convert_result.error_message),
              std::move(convert_result.error_message_args)};
    }
    if (!convert_result.data.has_value()) {
      spdlog::warn("GrpcHandlerInterface ConvertToServerData data missing: method={}", method_name_);
      return Result<void>{ErrorCode::kInternalError, "data is null"};
    }

    /// 提取转换后的业务层数据结构体
    server_data = std::move(convert_result.data.value());

    /// 步骤2: 参数合法性校验
    const auto validate_result = ValidateParams(server_data.value());
    if (validate_result.error_code != ErrorCode::kSuccess) {
      spdlog::warn(
        "GrpcHandlerInterface ValidateParams failed: method={}, {}", method_name_, validate_result.error_message);
      return validate_result;
    }

    /// 步骤3: 前置条件检查
    const auto precond_result = CheckPreconditions(server_data.value());
    if (precond_result.error_code != ErrorCode::kSuccess) {
      spdlog::warn(
        "GrpcHandlerInterface CheckPreconditions failed: method={}, {}", method_name_, precond_result.error_message);
      return precond_result;
    }

    /// 步骤4: 执行业务核心逻辑【失败时回滚】
    rollback_on_exception = true;
    const auto exec_result = ExecuteBusiness(server_data.value());
    if (exec_result.error_code != ErrorCode::kSuccess) {
      spdlog::error(
        "GrpcHandlerInterface ExecuteBusiness failed: method={}, {}", method_name_, exec_result.error_message);
      Rollback(server_data.value());
      rollback_on_exception = false;
      return exec_result;
    }

    /// 步骤5: 校验操作是否真正生效落地【失败时回滚】
    const auto verify_result = VerifyExecutionEffective(server_data.value());
    if (verify_result.error_code != ErrorCode::kSuccess) {
      spdlog::error("GrpcHandlerInterface VerifyExecutionEffective failed: method={}, {}",
                    method_name_,
                    verify_result.error_message);
      Rollback(server_data.value());
      rollback_on_exception = false;
      return verify_result;
    }

    /// 步骤6: 通知其他服务【失败不会回滚；异常路径也不回滚】
    rollback_on_exception = false;
    const auto notify_result = NotifyService(server_data.value());
    if (notify_result.error_code != ErrorCode::kSuccess) {
      spdlog::warn(
        "GrpcHandlerInterface NotifyService failed: method={}, {}", method_name_, notify_result.error_message);
    }

    /// 步骤7: 构造最终返回结果【失败时回滚】
    rollback_on_exception = true;
    const auto build_result = BuildResponse(server_data.value(), response);
    if (build_result.error_code != ErrorCode::kSuccess) {
      spdlog::error(
        "GrpcHandlerInterface BuildResponse failed: method={}, {}", method_name_, build_result.error_message);
      Rollback(server_data.value());
      rollback_on_exception = false;
      return build_result;
    }

    rollback_on_exception = false;
    /// 全部流程执行成功
    return Result<void>{ErrorCode::kSuccess, "success"};
  } catch (const std::exception& ex) {
    /// 捕获异常，防止服务崩溃
    spdlog::error("GrpcHandlerInterface exception: method={}, {}", method_name_, ex.what());
    if (rollback_on_exception && server_data.has_value()) {
      try {
        Rollback(server_data.value());
      } catch (const std::exception& rollback_ex) {
        spdlog::error("GrpcHandlerInterface rollback exception: method={}, {}", method_name_, rollback_ex.what());
      }
    }
    return Result<void>{ErrorCode::kSystemError, "handler run exception"};
  }
}

}  // namespace qtrade::framework::grpc

#endif  // QTRADE_FRAMEWORK_GRPC_GRPC_HANDLER_INTERFACE_HPP_
