/// @file      stub_trader_api.hpp
/// @brief     Mock TraderApi 适配器（Target: qtrade_sdk::trader::TraderApi）
/// @details   提供可用于演示和测试的内存交易接口实现。
/// @author    qtrade
/// @date      2026-07-19
/// @copyright Copyright (c) 2026 qtrade.
#ifndef QTRADE_TEST_STUB_TRADER_API_HPP_
#define QTRADE_TEST_STUB_TRADER_API_HPP_

#include "stubs/stub_trader_spi.hpp"

#include <qtrade/sdk/trader/trader_api.hpp>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace qtrade::test::stub {

/// @brief 测试用 Stub 交易 API 适配器。
/// @details 模拟交易连接、订单及成交回调，实现 qtrade SDK 交易接口。
class StubTraderApi final : public qtrade_sdk::trader::TraderApi {
 public:
  /// @brief 构造 测试用 Stub 交易 API 适配器。
  StubTraderApi();
  /// @brief 析构 测试用 Stub 交易 API 适配器。
  ~StubTraderApi() override;

  /// @copydoc qtrade_sdk::trader::TraderApi::SetCpuAffinity
  void SetCpuAffinity(std::int32_t thread1_cpu_core_id, std::int32_t thread2_cpu_core_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::GetErrMsgByCode
  std::string GetErrMsgByCode(std::int32_t error_code, std::uint64_t session_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::Release
  void Release() override;

  /// @copydoc qtrade_sdk::trader::TraderApi::GetTradingDay
  std::string GetTradingDay() const override;

  /// @copydoc qtrade_sdk::trader::TraderApi::RegisterSpi
  void RegisterSpi(qtrade_sdk::trader::TraderSpi& spi) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::UnregisterSpi
  void UnregisterSpi() override;

  /// @copydoc qtrade_sdk::trader::TraderApi::GetApiLastError
  const qtrade_sdk::trader::RspInfo* GetApiLastError() const override;

  /// @copydoc qtrade_sdk::trader::TraderApi::GetApiVersion
  std::string GetApiVersion() const override;

  /// @copydoc qtrade_sdk::trader::TraderApi::GetClientIDByBrokerOrderId
  std::uint8_t GetClientIDByBrokerOrderId(std::uint64_t broker_order_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::GetAccountByBrokerOrderId
  std::string GetAccountByBrokerOrderId(std::uint64_t broker_order_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::SubscribePublicTopic
  void SubscribePublicTopic(qtrade_sdk::trader::ResumeType resume_type) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::SetSoftwareVersion
  void SetSoftwareVersion(const std::string& version) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::SetHeartBeatInterval
  void SetHeartBeatInterval(std::uint32_t interval) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::Connect
  qtrade::ErrorCode Connect(const qtrade_sdk::trader::ConnectRequest& request) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::Disconnect
  void Disconnect() override;

  /// @copydoc qtrade_sdk::trader::TraderApi::IsConnected
  bool IsConnected() const override;

  /// @copydoc qtrade_sdk::trader::TraderApi::Login
  std::uint64_t Login(const std::string& ip,
                      int port,
                      const std::string& user,
                      const std::string& password,
                      qtrade_sdk::trader::ProtocolType sock_type,
                      const std::string& local_ip = {},
                      const qtrade_sdk::trader::UserTerminalInfo* terminal_info = nullptr) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::Logout
  int Logout(std::uint64_t session_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::IsServerRestart
  bool IsServerRestart(std::uint64_t session_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::InsertOrder
  std::uint64_t InsertOrder(const qtrade_sdk::trader::OrderRequest& order, std::uint64_t session_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::SendOrder
  qtrade::ErrorCode SendOrder(const qtrade_sdk::trader::OrderRequest& request) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::CancelOrder
  std::uint64_t CancelOrder(std::uint64_t broker_order_id, std::uint64_t session_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::CancelOrder
  qtrade::ErrorCode CancelOrder(const qtrade_sdk::trader::CancelOrderRequest& request) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::QueryOrderByBrokerOrderId
  int QueryOrderByBrokerOrderId(std::uint64_t broker_order_id, std::uint64_t session_id, int request_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::QueryOrders
  int QueryOrders(const qtrade_sdk::trader::QueryOrdersRequest& query_param,
                  std::uint64_t session_id,
                  int request_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::QueryOrders
  qtrade::ErrorCode QueryOrders(const qtrade_sdk::trader::QueryOrdersRequest& request,
                                qtrade_sdk::trader::QueryOrdersResponse& response) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::QueryUnfinishedOrders
  int QueryUnfinishedOrders(std::uint64_t session_id, int request_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::QueryOrdersByPage
  int QueryOrdersByPage(const qtrade_sdk::trader::QueryOrdersByPageRequest& query_param,
                        std::uint64_t session_id,
                        int request_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::QueryTradesByBrokerOrderId
  int QueryTradesByBrokerOrderId(std::uint64_t broker_order_id, std::uint64_t session_id, int request_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::QueryTrades
  int QueryTrades(const qtrade_sdk::trader::QueryTradesRequest& query_param,
                  std::uint64_t session_id,
                  int request_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::QueryTrades
  qtrade::ErrorCode QueryTrades(const qtrade_sdk::trader::QueryTradesRequest& request,
                                qtrade_sdk::trader::QueryTradesResponse& response) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::QueryPosition
  int QueryPosition(const std::string& ticker,
                    std::uint64_t session_id,
                    int request_id,
                    qtrade_sdk::trader::MarketType market = qtrade_sdk::trader::MarketType::kInit) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::QueryPositions
  qtrade::ErrorCode QueryPositions(const qtrade_sdk::trader::QueryPositionRequest& request,
                                   qtrade_sdk::trader::QueryPositionResponse& response) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::QueryAsset
  int QueryAsset(std::uint64_t session_id, int request_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::QueryAsset
  qtrade::ErrorCode QueryAsset(const qtrade_sdk::trader::QueryAssetRequest& request,
                               qtrade_sdk::trader::QueryAssetResponse& response) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::FundTransfer
  std::uint64_t FundTransfer(const qtrade_sdk::trader::FundTransferRequest& fund_transfer,
                             std::uint64_t session_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::QueryOtherServerFund
  int QueryOtherServerFund(const qtrade_sdk::trader::FundQueryRequest& query_param,
                           std::uint64_t session_id,
                           int request_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::CreditQuotaTransfer
  std::uint64_t CreditQuotaTransfer(const qtrade_sdk::trader::QuotaTransferRequest& quota_transfer,
                                    std::uint64_t session_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::QueryCreditQuotaTransfer
  int QueryCreditQuotaTransfer(const qtrade_sdk::trader::QueryQuotaTransferLogRequest& query_param,
                               std::uint64_t session_id,
                               int request_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::QueryFundTransfer
  int QueryFundTransfer(const qtrade_sdk::trader::QueryFundTransferLogRequest& query_param,
                        std::uint64_t session_id,
                        int request_id) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::SetOrderCallback
  void SetOrderCallback(OrderCallback cb) override;

  /// @copydoc qtrade_sdk::trader::TraderApi::SetTradeCallback
  void SetTradeCallback(TradeCallback cb) override;

  /// @brief 设置发单后是否立即生成全量成交
  /// @param auto_fill true 保持默认立即成交行为
  void SetAutoFill(bool auto_fill);

 private:
  /// @brief 测试用 Stub 交易回调适配器。
  StubTraderSpi mock_spi_;
  /// @brief 已注册的 qtrade 交易回调对象。
  qtrade_sdk::trader::TraderSpi* spi_ = nullptr;
  /// @brief 最近一次 API 错误信息。
  qtrade_sdk::trader::RspInfo last_error_;
  /// @brief 订单事件回调。
  OrderCallback on_order_;
  /// @brief 成交事件回调。
  TradeCallback on_trade_;
  /// @brief 当前交易日。
  std::string trading_day_;
  /// @brief Mock API 版本。
  std::string api_version_ = "mock";
  /// @brief 当前账户标识。
  std::string account_id_;
  /// @brief 设置的软件版本。
  std::string software_version_;
  /// @brief 连接状态。
  bool connected_ = false;
  /// @brief 当前会话标识。
  std::uint64_t session_id_ = 0;
  /// @brief 下一个券商/通道委托号。
  std::uint64_t next_broker_order_id_ = 1;
  /// @brief 下一个成交标识。
  std::uint64_t next_trade_id_ = 1;
  /// @brief 是否发单后立即成交。
  std::atomic<bool> auto_fill_ = true;
  /// @brief 券商/通道委托号到最近订单快照的映射。
  std::unordered_map<std::uint64_t, qtrade_sdk::trader::Order> orders_;
  /// @brief 已生成的成交回报。
  std::vector<qtrade_sdk::trader::Trade> trades_;
  /// @brief 保护 Mock 订单表的互斥锁。
  std::mutex orders_mutex_;
};

/// @brief 创建 测试用 Stub 交易 API。
/// @return 用于演示与测试的交易 API 实例。
std::unique_ptr<qtrade_sdk::trader::TraderApi> CreateStubTraderApi();

}  // namespace qtrade::test::stub

#endif  // QTRADE_TEST_STUB_TRADER_API_HPP_
