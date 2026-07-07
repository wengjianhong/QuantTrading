/// @file      emt_trader_api.cpp
/// @brief     EMT TraderApi 适配器实现
#include "adapter/emt/trader/emt_trader_api.hpp"

namespace qtrade::adapter::trader {

namespace sdk = qtrade_sdk::trader;

EmtTraderApi::EmtTraderApi() = default;

EmtTraderApi::~EmtTraderApi() { Release(); }

void EmtTraderApi::SetCpuAffinity(std::int32_t thread1_cpu_core_id, std::int32_t thread2_cpu_core_id) {
  (void)thread1_cpu_core_id;
  (void)thread2_cpu_core_id;
  // TODO(EMT SDK): emt_api_->SetCpuAffinity(...);
}

std::string EmtTraderApi::GetErrMsgByCode(std::int32_t error_code, std::uint64_t session_id) {
  (void)error_code;
  (void)session_id;
  // TODO(EMT SDK): return emt_api_->GetErrMsgByCode(...);
  return {};
}

void EmtTraderApi::Release() {
  // TODO(EMT SDK): emt_api_->Release();
}

std::string EmtTraderApi::GetTradingDay() const {
  // TODO(EMT SDK): return emt_api_->GetTradingDay();
  return trading_day_;
}

void EmtTraderApi::RegisterSpi(sdk::TraderSpi& spi) {
  spi_ = &spi;
  emt_spi_.SetTarget(&spi);
  // TODO(EMT SDK): emt_api_->RegisterSpi(&emt_spi_);
}

void EmtTraderApi::UnregisterSpi() {
  spi_ = nullptr;
  emt_spi_.SetTarget(nullptr);
  // TODO(EMT SDK): emt_api_->UnregisterSpi();
}

const sdk::RspInfo* EmtTraderApi::GetApiLastError() const { return &last_error_; }

std::string EmtTraderApi::GetApiVersion() const {
  // TODO(EMT SDK): return emt_api_->GetApiVersion();
  return api_version_;
}

std::uint8_t EmtTraderApi::GetClientIDByEMTID(std::uint64_t order_emt_id) {
  (void)order_emt_id;
  return 0;
}

std::string EmtTraderApi::GetAccountByEMTID(std::uint64_t order_emt_id) {
  (void)order_emt_id;
  return account_id_;
}

void EmtTraderApi::SubscribePublicTopic(sdk::ResumeType resume_type) { (void)resume_type; }

void EmtTraderApi::SetSoftwareVersion(const std::string& version) { software_version_ = version; }

void EmtTraderApi::SetHeartBeatInterval(std::uint32_t interval) { (void)interval; }

qtrade::ErrorCode EmtTraderApi::Connect(const sdk::ConnectRequest& request) {
  (void)request;
  return qtrade::ErrorCode::kNotSupported;
}

void EmtTraderApi::Disconnect() {}

bool EmtTraderApi::IsConnected() const { return false; }

std::uint64_t EmtTraderApi::Login(const std::string& ip,
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
  return 0;
}

int EmtTraderApi::Logout(std::uint64_t session_id) {
  (void)session_id;
  return -1;
}

bool EmtTraderApi::IsServerRestart(std::uint64_t session_id) {
  (void)session_id;
  return false;
}

std::uint64_t EmtTraderApi::InsertOrder(const sdk::OrderRequest& order, std::uint64_t session_id) {
  (void)session_id;
  return order.order_emt_id;
}

qtrade::ErrorCode EmtTraderApi::SendOrder(const sdk::OrderRequest& request) {
  (void)request;
  return qtrade::ErrorCode::kNotSupported;
}

std::uint64_t EmtTraderApi::CancelOrder(std::uint64_t order_emt_id, std::uint64_t session_id) {
  (void)order_emt_id;
  (void)session_id;
  return 0;
}

qtrade::ErrorCode EmtTraderApi::CancelOrder(const sdk::CancelOrderRequest& request) {
  (void)request;
  return qtrade::ErrorCode::kNotSupported;
}

int EmtTraderApi::QueryOrderByEMTID(std::uint64_t order_emt_id, std::uint64_t session_id, int request_id) {
  (void)order_emt_id;
  (void)session_id;
  (void)request_id;
  return -1;
}

int EmtTraderApi::QueryOrders(const sdk::QueryOrdersRequest& query_param, std::uint64_t session_id, int request_id) {
  (void)query_param;
  (void)session_id;
  (void)request_id;
  return -1;
}

qtrade::ErrorCode EmtTraderApi::QueryOrders(const sdk::QueryOrdersRequest& request,
                                            sdk::QueryOrdersResponse& response) {
  (void)request;
  response.orders.clear();
  return qtrade::ErrorCode::kNotSupported;
}

int EmtTraderApi::QueryUnfinishedOrders(std::uint64_t session_id, int request_id) {
  (void)session_id;
  (void)request_id;
  return -1;
}

int EmtTraderApi::QueryOrdersByPage(const sdk::QueryOrdersByPageRequest& query_param,
                                    std::uint64_t session_id,
                                    int request_id) {
  (void)query_param;
  (void)session_id;
  (void)request_id;
  return -1;
}

int EmtTraderApi::QueryTradesByEMTID(std::uint64_t order_emt_id, std::uint64_t session_id, int request_id) {
  (void)order_emt_id;
  (void)session_id;
  (void)request_id;
  return -1;
}

int EmtTraderApi::QueryTrades(const sdk::QueryTradesRequest& query_param, std::uint64_t session_id, int request_id) {
  (void)query_param;
  (void)session_id;
  (void)request_id;
  return -1;
}

qtrade::ErrorCode EmtTraderApi::QueryTrades(const sdk::QueryTradesRequest& request,
                                            sdk::QueryTradesResponse& response) {
  (void)request;
  response.trades.clear();
  return qtrade::ErrorCode::kNotSupported;
}

int EmtTraderApi::QueryPosition(const std::string& ticker,
                                std::uint64_t session_id,
                                int request_id,
                                sdk::MarketType market) {
  (void)ticker;
  (void)session_id;
  (void)request_id;
  (void)market;
  return -1;
}

qtrade::ErrorCode EmtTraderApi::QueryPositions(const sdk::QueryPositionRequest& request,
                                               sdk::QueryPositionResponse& response) {
  (void)request;
  response.positions.clear();
  return qtrade::ErrorCode::kNotSupported;
}

int EmtTraderApi::QueryAsset(std::uint64_t session_id, int request_id) {
  (void)session_id;
  (void)request_id;
  return -1;
}

qtrade::ErrorCode EmtTraderApi::QueryAsset(const sdk::QueryAssetRequest& request, sdk::QueryAssetResponse& response) {
  (void)request;
  response.asset = {};
  return qtrade::ErrorCode::kNotSupported;
}

std::uint64_t EmtTraderApi::FundTransfer(const sdk::FundTransferRequest& fund_transfer, std::uint64_t session_id) {
  (void)fund_transfer;
  (void)session_id;
  return 0;
}

int EmtTraderApi::QueryOtherServerFund(const sdk::FundQueryRequest& query_param,
                                       std::uint64_t session_id,
                                       int request_id) {
  (void)query_param;
  (void)session_id;
  (void)request_id;
  return -1;
}

std::uint64_t EmtTraderApi::CreditQuotaTransfer(const sdk::QuotaTransferRequest& quota_transfer,
                                                std::uint64_t session_id) {
  (void)quota_transfer;
  (void)session_id;
  return 0;
}

int EmtTraderApi::QueryCreditQuotaTransfer(const sdk::QueryQuotaTransferLogRequest& query_param,
                                           std::uint64_t session_id,
                                           int request_id) {
  (void)query_param;
  (void)session_id;
  (void)request_id;
  return -1;
}

int EmtTraderApi::QueryFundTransfer(const sdk::QueryFundTransferLogRequest& query_param,
                                    std::uint64_t session_id,
                                    int request_id) {
  (void)query_param;
  (void)session_id;
  (void)request_id;
  return -1;
}

void EmtTraderApi::SetOrderCallback(OrderCallback cb) { on_order_ = std::move(cb); }

void EmtTraderApi::SetTradeCallback(TradeCallback cb) { on_trade_ = std::move(cb); }

}  // namespace qtrade::adapter::trader
