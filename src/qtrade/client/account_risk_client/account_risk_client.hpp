/// @file account_risk_client.hpp
/// @brief 引擎到账户硬风控服务的 E 段客户端
#ifndef QTRADE_CLIENT_ACCOUNT_RISK_CLIENT_HPP_
#define QTRADE_CLIENT_ACCOUNT_RISK_CLIENT_HPP_

#include <qtrade/error_code/error_codes.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace qtrade::account_risk::v1 {
class ReserveOrderResponse;
class ReleaseOrderResponse;
}  // namespace qtrade::account_risk::v1
namespace qtrade_sdk::trader {
struct OrderRequest;
}

namespace qtrade::client {

struct AccountRiskClientOptions {
  std::string server_address;
  std::string tenant_id;
  std::string account_id;
  std::string engine_id;
  int timeout_ms = 3;
};

class AccountRiskClient {
 public:
  AccountRiskClient();
  ~AccountRiskClient();
  ErrorCode Init(const AccountRiskClientOptions& options);
  void Shutdown();
  [[nodiscard]] bool IsInitialized() const;
  ErrorCode ReserveOrder(const std::string& order_id,
                         const qtrade_sdk::trader::OrderRequest& request,
                         std::uint64_t risk_config_version,
                         qtrade::account_risk::v1::ReserveOrderResponse& response);
  ErrorCode ReleaseOrder(const std::string& order_id,
                         int reason,
                         qtrade::account_risk::v1::ReleaseOrderResponse& response);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace qtrade::client

#endif  // QTRADE_CLIENT_ACCOUNT_RISK_CLIENT_HPP_
