/// @file      emt_trader_api.hpp
/// @brief     EMT TraderApi 适配器（Target: qtrade_sdk::trader::TraderApi）
/// @details   将 qtrade SDK 的交易接口适配至 EMT 交易 API。
/// @author    qtrade
/// @date      2026-07-19
/// @copyright Copyright (c) 2026 qtrade.
#ifndef QTRADE_ADAPTER_EMT_TRADER_API_HPP_
#define QTRADE_ADAPTER_EMT_TRADER_API_HPP_

#include "qtrade_sdk/emt/trader/emt_trader_spi.hpp"

#include <qtrade_sdk/trader/trader_api.hpp>

#include <cstdint>
#include <functional>
#include <string>

namespace qtrade::adapter::trader {

/// @brief EMT 交易 API 适配器。
/// @details 接入 EMT SDK 后持有并转发调用；实现 qtrade SDK 的交易接口。
class EmtTraderApi final : public qtrade_sdk::trader::TraderApi {
 public:
  /// @brief 构造 EMT 交易 API 适配器。
  EmtTraderApi();
  /// @brief 析构 EMT 交易 API 适配器。
  ~EmtTraderApi() override;

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

 private:
  /// @brief EMT 交易回调适配器。
  EmtTraderSpi emt_spi_;
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
  /// @brief EMT API 版本。
  std::string api_version_{"emt"};
  /// @brief 当前账户标识。
  std::string account_id_;
  /// @brief 设置给 EMT API 的软件版本。
  std::string software_version_;
  /// @brief 待接入的 EMT 原生交易 API。
  /// @details TODO(EMT SDK): EMT::API::TraderApi* emt_api_ = nullptr;
};

}  // namespace qtrade::adapter::trader

#endif  // QTRADE_ADAPTER_EMT_TRADER_API_HPP_
