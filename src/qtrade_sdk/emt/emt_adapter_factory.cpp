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

/// @brief 写入可选错误码输出
/// @param out_error 输出指针；可为 nullptr
/// @param code 错误码
void SetOutError(ErrorCode* out_error, ErrorCode code) {
  if (out_error != nullptr) {
    *out_error = code;
  }
}

}  // namespace

std::optional<EmtAdapterBundle> CreateEmtAdapters(qtrade::client::AccountClient& account_client,
                                                  const std::string& tenant_id,
                                                  const std::string& engine_id,
                                                  const std::string& account_id,
                                                  const std::string& quote_connection_string,
                                                  ErrorCode* out_error) {
  if (!account_client.IsInitialized()) {
    SetOutError(out_error, ErrorCode::kNotInitialized);
    return std::nullopt;
  }
  if (account_id.empty() || quote_connection_string.empty()) {
    SetOutError(out_error, ErrorCode::kInternalError);
    return std::nullopt;
  }

  // 1. 拉取账户凭证
  qtrade::account::v1::GetCredentialRequest credential_request;
  credential_request.set_tenant_id(tenant_id);
  credential_request.set_engine_id(engine_id);
  credential_request.set_account_id(account_id);
  qtrade::account::v1::GetCredentialResponse credential_response;
  if (const auto result = account_client.GetCredential(credential_request, credential_response);
      result != ErrorCode::kSuccess) {
    SetOutError(out_error, result);
    return std::nullopt;
  }

  const auto& credential = credential_response.credential();
  if (credential.account_id() != account_id || credential.connection_string().empty() ||
      credential.password().empty()) {
    SetOutError(out_error, ErrorCode::kInternalError);
    return std::nullopt;
  }

  // 2. 构造适配器与连接请求
  EmtAdapterBundle bundle;
  bundle.trader_request.broker_id = credential.broker_id();
  bundle.trader_request.account_id = credential.account_id();
  bundle.trader_request.connection_string = credential.connection_string();
  bundle.trader_request.password = credential.password();
  bundle.quote_request.name = "emt";
  bundle.quote_request.connection_string = quote_connection_string;
  bundle.quote_request.user = credential.account_id();
  bundle.quote_request.password = credential.password();
  bundle.quote_api = std::make_unique<qtrade::adapter::quote::EmtQuoteApi>();
  bundle.trader_api = std::make_unique<qtrade::adapter::trader::EmtTraderApi>();
  SetOutError(out_error, ErrorCode::kSuccess);
  return bundle;
}

}  // namespace qtrade::adapter::emt
