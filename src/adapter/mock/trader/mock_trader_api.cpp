/// @file      mock_trader_api.cpp
/// @brief     Mock TraderApi 适配器实现
#include "adapter/mock/trader/mock_trader_api.hpp"

namespace qtrade::adapter::mock::trader {

namespace sdk = qtrade_sdk::trader;

MockTraderApi::MockTraderApi() = default;

MockTraderApi::~MockTraderApi() { Release(); }

void MockTraderApi::SetCpuAffinity(std::int32_t thread1_cpu_core_id, std::int32_t thread2_cpu_core_id) {
  (void)thread1_cpu_core_id;
  (void)thread2_cpu_core_id;
}

std::string MockTraderApi::GetErrMsgByCode(std::int32_t error_code, std::uint64_t session_id) {
  (void)error_code;
  (void)session_id;
  return {};
}

void MockTraderApi::Release() { Disconnect(); }

std::string MockTraderApi::GetTradingDay() const { return trading_day_; }

void MockTraderApi::RegisterSpi(sdk::TraderSpi& spi) {
  spi_ = &spi;
  mock_spi_.SetTarget(&spi);
}

void MockTraderApi::UnregisterSpi() {
  spi_ = nullptr;
  mock_spi_.SetTarget(nullptr);
}

const sdk::RspInfo* MockTraderApi::GetApiLastError() const { return &last_error_; }

std::string MockTraderApi::GetApiVersion() const { return api_version_; }

std::uint8_t MockTraderApi::GetClientIDByEMTID(std::uint64_t order_emt_id) {
  (void)order_emt_id;
  return 0;
}

std::string MockTraderApi::GetAccountByEMTID(std::uint64_t order_emt_id) {
  (void)order_emt_id;
  return account_id_;
}

void MockTraderApi::SubscribePublicTopic(sdk::ResumeType resume_type) { (void)resume_type; }

void MockTraderApi::SetSoftwareVersion(const std::string& version) { software_version_ = version; }

void MockTraderApi::SetHeartBeatInterval(std::uint32_t interval) { (void)interval; }

qtrade::ErrorCode MockTraderApi::Connect(const sdk::ConnectRequest& request) {
  (void)request;
  connected_ = true;
  mock_spi_.PublishConnected();
  return qtrade::ErrorCode::kSuccess;
}

void MockTraderApi::Disconnect() { connected_ = false; }

bool MockTraderApi::IsConnected() const { return connected_; }

std::uint64_t MockTraderApi::Login(const std::string& ip,
                                   int port,
                                   const std::string& user,
                                   const std::string& password,
                                   sdk::ProtocolType sock_type,
                                   const std::string& local_ip,
                                   const sdk::UserTerminalInfo* terminal_info) {
  (void)ip;
  (void)port;
  (void)user;
  (void)password;
  (void)sock_type;
  (void)local_ip;
  (void)terminal_info;
  session_id_ = 1;
  mock_spi_.PublishConnected();
  return session_id_;
}

int MockTraderApi::Logout(std::uint64_t session_id) {
  (void)session_id;
  session_id_ = 0;
  return 0;
}

bool MockTraderApi::IsServerRestart(std::uint64_t session_id) {
  (void)session_id;
  return false;
}

std::uint64_t MockTraderApi::InsertOrder(const sdk::OrderRequest& order, std::uint64_t session_id) {
  (void)session_id;
  return order.order_emt_id;
}

qtrade::ErrorCode MockTraderApi::SendOrder(const sdk::OrderRequest& request) {
  if (on_order_) {
    sdk::Order order;
    order.instrument = request.instrument;
    order.order_emt_id = request.order_emt_id;
    on_order_(order);
  }
  return qtrade::ErrorCode::kSuccess;
}

std::uint64_t MockTraderApi::CancelOrder(std::uint64_t order_emt_id, std::uint64_t session_id) {
  (void)order_emt_id;
  (void)session_id;
  return 0;
}

qtrade::ErrorCode MockTraderApi::CancelOrder(const sdk::CancelOrderRequest& request) {
  (void)request;
  return qtrade::ErrorCode::kSuccess;
}

int MockTraderApi::QueryOrderByEMTID(std::uint64_t order_emt_id, std::uint64_t session_id, int request_id) {
  (void)order_emt_id;
  (void)session_id;
  (void)request_id;
  return -1;
}

int MockTraderApi::QueryOrders(const sdk::QueryOrdersRequest& query_param, std::uint64_t session_id, int request_id) {
  (void)query_param;
  (void)session_id;
  (void)request_id;
  return -1;
}

qtrade::ErrorCode MockTraderApi::QueryOrders(const sdk::QueryOrdersRequest& request,
                                             sdk::QueryOrdersResponse& response) {
  (void)request;
  response.orders.clear();
  return qtrade::ErrorCode::kNotSupported;
}

int MockTraderApi::QueryUnfinishedOrders(std::uint64_t session_id, int request_id) {
  (void)session_id;
  (void)request_id;
  return -1;
}

int MockTraderApi::QueryOrdersByPage(const sdk::QueryOrdersByPageRequest& query_param,
                                     std::uint64_t session_id,
                                     int request_id) {
  (void)query_param;
  (void)session_id;
  (void)request_id;
  return -1;
}

int MockTraderApi::QueryTradesByEMTID(std::uint64_t order_emt_id, std::uint64_t session_id, int request_id) {
  (void)order_emt_id;
  (void)session_id;
  (void)request_id;
  return -1;
}

int MockTraderApi::QueryTrades(const sdk::QueryTradesRequest& query_param, std::uint64_t session_id, int request_id) {
  (void)query_param;
  (void)session_id;
  (void)request_id;
  return -1;
}

qtrade::ErrorCode MockTraderApi::QueryTrades(const sdk::QueryTradesRequest& request,
                                             sdk::QueryTradesResponse& response) {
  (void)request;
  response.trades.clear();
  return qtrade::ErrorCode::kNotSupported;
}

int MockTraderApi::QueryPosition(const std::string& ticker,
                                 std::uint64_t session_id,
                                 int request_id,
                                 sdk::MarketType market) {
  (void)ticker;
  (void)session_id;
  (void)request_id;
  (void)market;
  return -1;
}

qtrade::ErrorCode MockTraderApi::QueryPositions(const sdk::QueryPositionRequest& request,
                                                sdk::QueryPositionResponse& response) {
  (void)request;
  response.positions.clear();
  return qtrade::ErrorCode::kNotSupported;
}

int MockTraderApi::QueryAsset(std::uint64_t session_id, int request_id) {
  (void)session_id;
  (void)request_id;
  return -1;
}

qtrade::ErrorCode MockTraderApi::QueryAsset(const sdk::QueryAssetRequest& request, sdk::QueryAssetResponse& response) {
  (void)request;
  response.asset = {};
  return qtrade::ErrorCode::kNotSupported;
}

std::uint64_t MockTraderApi::FundTransfer(const sdk::FundTransferRequest& fund_transfer, std::uint64_t session_id) {
  (void)fund_transfer;
  (void)session_id;
  return 0;
}

int MockTraderApi::QueryOtherServerFund(const sdk::FundQueryRequest& query_param,
                                        std::uint64_t session_id,
                                        int request_id) {
  (void)query_param;
  (void)session_id;
  (void)request_id;
  return -1;
}

std::uint64_t MockTraderApi::CreditQuotaTransfer(const sdk::QuotaTransferRequest& quota_transfer,
                                                 std::uint64_t session_id) {
  (void)quota_transfer;
  (void)session_id;
  return 0;
}

int MockTraderApi::QueryCreditQuotaTransfer(const sdk::QueryQuotaTransferLogRequest& query_param,
                                            std::uint64_t session_id,
                                            int request_id) {
  (void)query_param;
  (void)session_id;
  (void)request_id;
  return -1;
}

int MockTraderApi::QueryFundTransfer(const sdk::QueryFundTransferLogRequest& query_param,
                                     std::uint64_t session_id,
                                     int request_id) {
  (void)query_param;
  (void)session_id;
  (void)request_id;
  return -1;
}

void MockTraderApi::SetOrderCallback(OrderCallback cb) { on_order_ = std::move(cb); }

void MockTraderApi::SetTradeCallback(TradeCallback cb) { on_trade_ = std::move(cb); }

}  // namespace qtrade::adapter::mock::trader

namespace qtrade::adapter::mock::trader {

std::unique_ptr<qtrade_sdk::trader::TraderApi> CreateMockTraderApi() { return std::make_unique<MockTraderApi>(); }

}  // namespace qtrade::adapter::mock::trader
