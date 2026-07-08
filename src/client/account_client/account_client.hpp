/// @file      account_client.hpp
/// @brief     交易账户凭证客户端
/// @details   引擎出站 gRPC GetCredential（启动阶段按需拉取）
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_CLIENT_ACCOUNT_CLIENT_HPP_
#define QTRADE_TRADING_CLIENT_ACCOUNT_CLIENT_HPP_

#include <qtrade/error_code/error_codes.hpp>

#include <memory>
#include <string>

namespace qtrade::account::v1 {
class GetCredentialResponse;
}

namespace qtrade::client {

struct AccountClientOptions {
  std::string server_address;
  std::string tenant_id = "default";
  std::string engine_id = "default";
};

class AccountClient {
 public:
  AccountClient();
  ~AccountClient();

  AccountClient(const AccountClient&) = delete;
  AccountClient& operator=(const AccountClient&) = delete;

  ErrorCode Init(const AccountClientOptions& options);
  void Shutdown();

  ErrorCode GetCredential(const std::string& account_id, qtrade::account::v1::GetCredentialResponse& response);

  [[nodiscard]] bool IsInitialized() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace qtrade::client

#endif  // QTRADE_TRADING_CLIENT_ACCOUNT_CLIENT_HPP_
