/// @file      fund_struct.hpp
/// @brief     资金划拨相关结构体
/// @details   参考 EMT_API 的 eoms_api_fund_struct.h，定义资金划转、额度调拨与资金查询结构体。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SDK_TRADER_FUND_STRUCT_HPP_
#define QTRADE_SDK_TRADER_FUND_STRUCT_HPP_

#include <qtrade_sdk/trader/trader_types.hpp>

#include <array>
#include <cstdint>
#include <string>

namespace qtrade_sdk::trader {

/// @brief 两地分仓信用额度划拨请求，对应 EMTQuotaTransferReq。
struct QuotaTransferRequest {
  /// 额度划拨编号，发单前无需填写。
  std::uint64_t serial_id = 0;
  /// 资金账户代码。
  std::string fund_account;
  /// 划拨金额，单位元。
  double amount = 0.0;
  /// 额度划拨方向。
  QuotaTransferType transfer_type = QuotaTransferType::kUnknown;
};

/// @brief 用户资金划转请求，对应 EMTFundTransferReq。
struct FundTransferRequest {
  /// 资金内转编号，发单前无需填写。
  std::uint64_t serial_id = 0;
  /// 资金账户代码。
  std::string fund_account;
  /// 资金账户密码。
  std::string password;
  /// 划拨金额，单位元。
  double amount = 0.0;
  /// 资金流转方向。
  FundTransferType transfer_type = FundTransferType::kUnknown;
};

/// @brief 用户资金查询请求，对应 EMTFundQueryReq。
struct FundQueryRequest {
  /// 资金账户代码。
  std::string fund_account;
  /// 资金账户密码。
  std::string password;
  /// 资金查询类型。
  FundQueryType query_type = FundQueryType::kUnknown;
  /// 预留字段。
  std::array<std::uint64_t, 4> reserved{};
};

/// @brief 用户资金查询响应，对应 EMTFundQueryRsp。
struct FundQueryResponse {
  /// 查询到的金额，单位元。
  double amount = 0.0;
  /// 资金查询类型。
  FundQueryType query_type = FundQueryType::kUnknown;
  /// 预留字段。
  std::array<std::uint64_t, 4> reserved{};
};

/// @brief 资金内转流水查询请求，对应 EMTQueryFundTransferLogReq。
struct QueryFundTransferLogRequest {
  /// 资金内转编号；0 表示查询全部。
  std::uint64_t serial_id = 0;
};

/// @brief 融券额度调拨流水查询请求，对应 EMTQueryQuotaTransferLogReq。
struct QueryQuotaTransferLogRequest {
  /// 额度划拨编号；0 表示查询全部。
  std::uint64_t serial_id = 0;
};

/// @brief 资金内转流水通知，对应 EMTFundTransferNotice。
struct FundTransferNotice {
  /// 资金内转编号。
  std::uint64_t serial_id = 0;
  /// 内转类型。
  FundTransferType transfer_type = FundTransferType::kUnknown;
  /// 划拨金额，单位元。
  double amount = 0.0;
  /// 操作结果。
  FundOperStatus oper_status = FundOperStatus::kUnknown;
  /// 操作时间，格式 YYYYMMDDHHMMSSsss。
  std::uint64_t transfer_time = 0;
};

/// @brief 两地分仓额度内转流水通知，对应 EMTQuotaTransferNotice。
struct QuotaTransferNotice {
  /// 额度划拨编号。
  std::uint64_t serial_id = 0;
  /// 内转类型。
  QuotaTransferType transfer_type = QuotaTransferType::kUnknown;
  /// 划拨金额，单位元。
  double amount = 0.0;
  /// 操作结果。
  QuotaOperStatus oper_status = QuotaOperStatus::kUnknown;
  /// 操作时间，格式 YYYYMMDDHHMMSSsss。
  std::uint64_t transfer_time = 0;
};

/// @brief 资金划拨通知确认类型。
using FundTransferAck = FundTransferNotice;
/// @brief 额度划拨通知确认类型。
using QuotaTransferAck = QuotaTransferNotice;

}  // namespace qtrade_sdk::trader

#endif  // QTRADE_SDK_TRADER_FUND_STRUCT_HPP_
