/// @file      emt_adapter_factory.cpp
/// @brief     EMT 行情/交易适配器装配实现
/// @author    wengjianhong
/// @date      2026-08-03
/// @copyright CC BY-NC-SA 4.0
#include "qtrade_sdk/emt/emt_adapter_factory.hpp"

#include "qtrade_sdk/emt/quote/emt_quote_api.hpp"
#include "qtrade_sdk/emt/trader/emt_trader_api.hpp"

namespace qtrade::adapter::emt {
namespace {

void SetOutError(ErrorCode* out_error, ErrorCode code) {
  if (out_error != nullptr) {
    *out_error = code;
  }
}

}  // namespace

std::optional<EmtAdapterBundle> CreateEmtAdapters(qtrade::account::IAccountBridge& account_bridge,
                                                  const std::string& tenant_id,
                                                  const std::string& engine_id,
                                                  const std::string& account_id,
                                                  const std::string& quote_connection_string,
                                                  ErrorCode* out_error) {
  if (account_id.empty() || quote_connection_string.empty()) {
    SetOutError(out_error, ErrorCode::kInternalError);
    return std::nullopt;
  }

  const auto credential_result = account_bridge.GetCredential(tenant_id, account_id, engine_id);
  if (credential_result.error_code != ErrorCode::kSuccess || !credential_result.data.has_value()) {
    SetOutError(out_error, credential_result.error_code);
    return std::nullopt;
  }

  const auto& credential = *credential_result.data;
  if (credential.account_id != account_id || credential.connection_string.empty() || credential.password.empty()) {
    SetOutError(out_error, ErrorCode::kInternalError);
    return std::nullopt;
  }

  EmtAdapterBundle bundle;
  bundle.trader_request.broker_id = credential.broker_id;
  bundle.trader_request.account_id = credential.account_id;
  bundle.trader_request.connection_string = credential.connection_string;
  bundle.trader_request.password = credential.password;
  bundle.quote_request.name = "emt";
  bundle.quote_request.connection_string = quote_connection_string;
  bundle.quote_request.user = credential.account_id;
  bundle.quote_request.password = credential.password;
  bundle.quote_api = std::make_unique<qtrade::adapter::quote::EmtQuoteApi>();
  bundle.trader_api = std::make_unique<qtrade::adapter::trader::EmtTraderApi>();
  SetOutError(out_error, ErrorCode::kSuccess);
  return bundle;
}

}  // namespace qtrade::adapter::emt
