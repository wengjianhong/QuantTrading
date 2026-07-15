/// @file      mock_trader_api.hpp
/// @brief     Mock TraderApi 适配器（Target: qtrade_sdk::trader::TraderApi）
#ifndef QTRADE_ADAPTER_MOCK_TRADER_API_HPP_
#define QTRADE_ADAPTER_MOCK_TRADER_API_HPP_

#include "qtrade_sdk/mock/trader/mock_trader_spi.hpp"

#include <qtrade_sdk/trader/trader_api.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace qtrade::adapter::mock::trader {

class MockTraderApi final : public qtrade_sdk::trader::TraderApi {
 public:
  MockTraderApi();
  ~MockTraderApi() override;

  void SetCpuAffinity(std::int32_t thread1_cpu_core_id, std::int32_t thread2_cpu_core_id) override;

  std::string GetErrMsgByCode(std::int32_t error_code, std::uint64_t session_id) override;

  void Release() override;

  std::string GetTradingDay() const override;

  void RegisterSpi(qtrade_sdk::trader::TraderSpi& spi) override;

  void UnregisterSpi() override;

  const qtrade_sdk::trader::RspInfo* GetApiLastError() const override;

  std::string GetApiVersion() const override;

  std::uint8_t GetClientIDByEMTID(std::uint64_t order_emt_id) override;

  std::string GetAccountByEMTID(std::uint64_t order_emt_id) override;

  void SubscribePublicTopic(qtrade_sdk::trader::ResumeType resume_type) override;

  void SetSoftwareVersion(const std::string& version) override;

  void SetHeartBeatInterval(std::uint32_t interval) override;

  qtrade::ErrorCode Connect(const qtrade_sdk::trader::ConnectRequest& request) override;

  void Disconnect() override;

  bool IsConnected() const override;

  std::uint64_t Login(const std::string& ip,
                      int port,
                      const std::string& user,
                      const std::string& password,
                      qtrade_sdk::trader::ProtocolType sock_type,
                      const std::string& local_ip = {},
                      const qtrade_sdk::trader::UserTerminalInfo* terminal_info = nullptr) override;

  int Logout(std::uint64_t session_id) override;

  bool IsServerRestart(std::uint64_t session_id) override;

  std::uint64_t InsertOrder(const qtrade_sdk::trader::OrderRequest& order, std::uint64_t session_id) override;

  qtrade::ErrorCode SendOrder(const qtrade_sdk::trader::OrderRequest& request) override;

  std::uint64_t CancelOrder(std::uint64_t order_emt_id, std::uint64_t session_id) override;

  qtrade::ErrorCode CancelOrder(const qtrade_sdk::trader::CancelOrderRequest& request) override;

  int QueryOrderByEMTID(std::uint64_t order_emt_id, std::uint64_t session_id, int request_id) override;

  int QueryOrders(const qtrade_sdk::trader::QueryOrdersRequest& query_param,
                  std::uint64_t session_id,
                  int request_id) override;

  qtrade::ErrorCode QueryOrders(const qtrade_sdk::trader::QueryOrdersRequest& request,
                                qtrade_sdk::trader::QueryOrdersResponse& response) override;

  int QueryUnfinishedOrders(std::uint64_t session_id, int request_id) override;

  int QueryOrdersByPage(const qtrade_sdk::trader::QueryOrdersByPageRequest& query_param,
                        std::uint64_t session_id,
                        int request_id) override;

  int QueryTradesByEMTID(std::uint64_t order_emt_id, std::uint64_t session_id, int request_id) override;

  int QueryTrades(const qtrade_sdk::trader::QueryTradesRequest& query_param,
                  std::uint64_t session_id,
                  int request_id) override;

  qtrade::ErrorCode QueryTrades(const qtrade_sdk::trader::QueryTradesRequest& request,
                                qtrade_sdk::trader::QueryTradesResponse& response) override;

  int QueryPosition(const std::string& ticker,
                    std::uint64_t session_id,
                    int request_id,
                    qtrade_sdk::trader::MarketType market = qtrade_sdk::trader::MarketType::kInit) override;

  qtrade::ErrorCode QueryPositions(const qtrade_sdk::trader::QueryPositionRequest& request,
                                   qtrade_sdk::trader::QueryPositionResponse& response) override;

  int QueryAsset(std::uint64_t session_id, int request_id) override;

  qtrade::ErrorCode QueryAsset(const qtrade_sdk::trader::QueryAssetRequest& request,
                               qtrade_sdk::trader::QueryAssetResponse& response) override;

  std::uint64_t FundTransfer(const qtrade_sdk::trader::FundTransferRequest& fund_transfer,
                             std::uint64_t session_id) override;

  int QueryOtherServerFund(const qtrade_sdk::trader::FundQueryRequest& query_param,
                           std::uint64_t session_id,
                           int request_id) override;

  std::uint64_t CreditQuotaTransfer(const qtrade_sdk::trader::QuotaTransferRequest& quota_transfer,
                                    std::uint64_t session_id) override;

  int QueryCreditQuotaTransfer(const qtrade_sdk::trader::QueryQuotaTransferLogRequest& query_param,
                               std::uint64_t session_id,
                               int request_id) override;

  int QueryFundTransfer(const qtrade_sdk::trader::QueryFundTransferLogRequest& query_param,
                        std::uint64_t session_id,
                        int request_id) override;

  void SetOrderCallback(OrderCallback cb) override;

  void SetTradeCallback(TradeCallback cb) override;

 private:
  MockTraderSpi mock_spi_;
  qtrade_sdk::trader::TraderSpi* spi_ = nullptr;
  qtrade_sdk::trader::RspInfo last_error_;
  OrderCallback on_order_;
  TradeCallback on_trade_;
  std::string trading_day_;
  std::string api_version_ = "mock";
  std::string account_id_;
  std::string software_version_;
  bool connected_ = false;
  std::uint64_t session_id_ = 0;
  std::uint64_t next_order_emt_id_ = 1;
  std::uint64_t next_trade_id_ = 1;
};

/// @brief 创建 Mock 交易 Api，供 demo 与测试使用。
std::unique_ptr<qtrade_sdk::trader::TraderApi> CreateMockTraderApi();

}  // namespace qtrade::adapter::mock::trader

#endif  // QTRADE_ADAPTER_MOCK_TRADER_API_HPP_
