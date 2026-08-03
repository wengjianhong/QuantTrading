/// @file      emt_adapter_factory.hpp
/// @brief     EMT 行情/交易适配器装配辅助
/// @details   经 account-service 拉取凭证后构造 Emt QuoteApi/TraderApi 并填充连接请求。
/// @author    wengjianhong
/// @date      2026-08-03
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ADAPTER_EMT_ADAPTER_FACTORY_HPP_
#define QTRADE_ADAPTER_EMT_ADAPTER_FACTORY_HPP_

#include "qtrade/client/account_client/account_client.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_sdk/quote/quote_api.hpp>
#include <qtrade_sdk/quote/quote_struct.hpp>
#include <qtrade_sdk/trader/trader_api.hpp>
#include <qtrade_sdk/trader/trader_struct.hpp>

#include <memory>
#include <optional>
#include <string>

namespace qtrade::adapter::emt {

/// @brief EMT 适配器与连接请求打包结果
struct EmtAdapterBundle {
  /// 行情 API
  std::unique_ptr<qtrade_sdk::quote::QuoteApi> quote_api;
  /// 交易 API
  std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api;
  /// 行情连接请求
  qtrade_sdk::quote::ConnectRequest quote_request;
  /// 交易连接请求
  qtrade_sdk::trader::ConnectRequest trader_request;
};

/// @brief 拉取账户凭证并构造 EMT 行情/交易适配器
/// @param account_client 已 Init 的 account-service 客户端
/// @param tenant_id 租户 ID
/// @param engine_id 引擎实例 ID
/// @param account_id 交易账户号（须与凭证一致）
/// @param quote_connection_string 行情连接串
/// @param out_error 失败时写入错误码；可为 nullptr
/// @return 成功返回装配结果；失败返回 nullopt
[[nodiscard]] std::optional<EmtAdapterBundle> CreateEmtAdapters(qtrade::client::AccountClient& account_client,
                                                                const std::string& tenant_id,
                                                                const std::string& engine_id,
                                                                const std::string& account_id,
                                                                const std::string& quote_connection_string,
                                                                ErrorCode* out_error = nullptr);

}  // namespace qtrade::adapter::emt

#endif  // QTRADE_ADAPTER_EMT_ADAPTER_FACTORY_HPP_
