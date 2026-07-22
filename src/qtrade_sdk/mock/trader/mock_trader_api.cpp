/// @file      mock_trader_api.cpp
/// @brief     Mock TraderApi 适配器实现
/// @details   实现模拟交易接口的连接、委托、查询及回调分发行为。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade_sdk/mock/trader/mock_trader_api.hpp"

namespace qtrade::adapter::mock::trader {

namespace sdk = qtrade_sdk::trader;

MockTraderApi::MockTraderApi() = default;

MockTraderApi::~MockTraderApi() {
  Release();
}

void MockTraderApi::SetCpuAffinity(std::int32_t thread1_cpu_core_id, std::int32_t thread2_cpu_core_id) {
  (void)thread1_cpu_core_id;
  (void)thread2_cpu_core_id;
}

std::string MockTraderApi::GetErrMsgByCode(std::int32_t error_code, std::uint64_t session_id) {
  (void)error_code;
  (void)session_id;
  return {};
}

void MockTraderApi::Release() {
  Disconnect();
}

std::string MockTraderApi::GetTradingDay() const {
  return trading_day_;
}

void MockTraderApi::RegisterSpi(sdk::TraderSpi& spi) {
  spi_ = &spi;
  mock_spi_.SetTarget(&spi);
}

void MockTraderApi::UnregisterSpi() {
  spi_ = nullptr;
  mock_spi_.SetTarget(nullptr);
}

const sdk::RspInfo* MockTraderApi::GetApiLastError() const {
  return &last_error_;
}

std::string MockTraderApi::GetApiVersion() const {
  return api_version_;
}

std::uint8_t MockTraderApi::GetClientIDByEMTID(std::uint64_t order_emt_id) {
  (void)order_emt_id;
  return 0;
}

std::string MockTraderApi::GetAccountByEMTID(std::uint64_t order_emt_id) {
  (void)order_emt_id;
  return account_id_;
}

void MockTraderApi::SubscribePublicTopic(sdk::ResumeType resume_type) {
  (void)resume_type;
}

void MockTraderApi::SetSoftwareVersion(const std::string& version) {
  software_version_ = version;
}

void MockTraderApi::SetHeartBeatInterval(std::uint32_t interval) {
  (void)interval;
}

qtrade::ErrorCode MockTraderApi::Connect(const sdk::ConnectRequest& request) {
  (void)request;
  connected_ = true;
  mock_spi_.PublishConnected();
  return qtrade::ErrorCode::kSuccess;
}

void MockTraderApi::Disconnect() {
  connected_ = false;
}

bool MockTraderApi::IsConnected() const {
  return connected_;
}

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
  if (!connected_) {
    return qtrade::ErrorCode::kConnectionError;
  }
  const std::uint64_t order_emt_id = request.order_emt_id != 0 ? request.order_emt_id : next_order_emt_id_++;
  sdk::Order order;
  order.client_order_id = request.client_order_id;
  order.instrument = request.instrument;
  order.market = request.market;
  order.order_emt_id = order_emt_id;
  order.price = request.price;
  order.volume = request.volume;
  order.left_volume = request.volume;
  order.side = request.side;
  order.position_effect = request.position_effect;
  order.business_type = request.business_type;
  order.status = sdk::OrderStatusType::kNotTradedQueueing;
  order.submit_status = sdk::OrderSubmitStatusType::kInsertAccepted;
  {
    std::lock_guard lock(orders_mutex_);
    orders_[order_emt_id] = order;
  }
  if (on_order_) {
    on_order_(order);
  }
  if (auto_fill_.load(std::memory_order_acquire) && on_trade_) {
    sdk::Trade trade;
    trade.trade_id = "MOCK-TRADE-" + std::to_string(next_trade_id_++);
    trade.order_emt_id = order_emt_id;
    trade.client_order_id = request.client_order_id;
    trade.instrument = request.instrument;
    trade.market = request.market;
    trade.price = request.price;
    trade.volume = request.volume;
    trade.trade_amount = request.price * static_cast<double>(request.volume);
    trade.side = request.side;
    trade.position_effect = request.position_effect;
    trade.business_type = request.business_type;
    on_trade_(trade);
    std::lock_guard lock(orders_mutex_);
    trades_.push_back(trade);
    auto& stored = orders_.at(order_emt_id);
    stored.traded_volume = stored.volume;
    stored.left_volume = 0;
    stored.trade_amount = trade.trade_amount;
    stored.status = sdk::OrderStatusType::kFilled;
  }
  return qtrade::ErrorCode::kSuccess;
}

std::uint64_t MockTraderApi::CancelOrder(std::uint64_t order_emt_id, std::uint64_t session_id) {
  (void)order_emt_id;
  (void)session_id;
  return 0;
}

qtrade::ErrorCode MockTraderApi::CancelOrder(const sdk::CancelOrderRequest& request) {
  if (!connected_) {
    return qtrade::ErrorCode::kConnectionError;
  }
  sdk::Order canceled;
  {
    std::lock_guard lock(orders_mutex_);
    const auto it = orders_.find(request.order_emt_id);
    if (it == orders_.end()) {
      return qtrade::ErrorCode::kNotFound;
    }
    if (it->second.status == sdk::OrderStatusType::kFilled || it->second.status == sdk::OrderStatusType::kCanceled ||
        it->second.status == sdk::OrderStatusType::kRejected) {
      return qtrade::ErrorCode::kSystemError;
    }
    it->second.order_id = request.order_id;
    it->second.status = sdk::OrderStatusType::kCanceled;
    it->second.submit_status = sdk::OrderSubmitStatusType::kCancelAccepted;
    canceled = it->second;
  }
  if (on_order_) {
    on_order_(canceled);
  }
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
  response.orders.clear();
  std::lock_guard lock(orders_mutex_);
  for (const auto& [order_emt_id, order] : orders_) {
    if ((!request.instrument.empty() && order.instrument != request.instrument) ||
        (request.order_emt_id != 0 && order_emt_id != request.order_emt_id) ||
        (request.status != sdk::OrderStatusType::kUnknown && order.status != request.status)) {
      continue;
    }
    response.orders.push_back(order);
  }
  return qtrade::ErrorCode::kSuccess;
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
  response.trades.clear();
  std::lock_guard lock(orders_mutex_);
  for (const auto& trade : trades_) {
    if ((!request.instrument.empty() && trade.instrument != request.instrument) ||
        (!request.order_id.empty() && trade.order_id != request.order_id) ||
        (request.order_emt_id != 0 && trade.order_emt_id != request.order_emt_id)) {
      continue;
    }
    response.trades.push_back(trade);
  }
  return qtrade::ErrorCode::kSuccess;
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
  response.positions.clear();
  std::unordered_map<std::string, sdk::Position> positions;
  {
    std::lock_guard lock(orders_mutex_);
    for (const auto& trade : trades_) {
      if (!request.instrument.empty() && request.instrument != trade.instrument) {
        continue;
      }
      auto& position = positions[trade.instrument];
      position.instrument = trade.instrument;
      position.market = trade.market;
      position.direction = sdk::PositionDirectionType::kNet;
      position.total_volume += trade.side == sdk::SideType::kBuy ? trade.volume : -trade.volume;
    }
  }
  for (auto& [instrument, position] : positions) {
    (void)instrument;
    if (position.total_volume < 0) {
      position.direction = sdk::PositionDirectionType::kShort;
      position.total_volume = -position.total_volume;
    } else {
      position.direction = sdk::PositionDirectionType::kLong;
    }
    response.positions.push_back(std::move(position));
  }
  return qtrade::ErrorCode::kSuccess;
}

int MockTraderApi::QueryAsset(std::uint64_t session_id, int request_id) {
  (void)session_id;
  (void)request_id;
  return -1;
}

qtrade::ErrorCode MockTraderApi::QueryAsset(const sdk::QueryAssetRequest& request, sdk::QueryAssetResponse& response) {
  response.asset = {};
  response.asset.account_id = request.account_id.empty() ? account_id_ : request.account_id;
  response.asset.total_asset = 1000000.0;
  response.asset.buying_power = 1000000.0;
  return qtrade::ErrorCode::kSuccess;
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

void MockTraderApi::SetOrderCallback(OrderCallback cb) {
  on_order_ = std::move(cb);
}

void MockTraderApi::SetTradeCallback(TradeCallback cb) {
  on_trade_ = std::move(cb);
}

void MockTraderApi::SetAutoFill(bool auto_fill) {
  auto_fill_.store(auto_fill, std::memory_order_release);
}

}  // namespace qtrade::adapter::mock::trader

namespace qtrade::adapter::mock::trader {

std::unique_ptr<qtrade_sdk::trader::TraderApi> CreateMockTraderApi() {
  return std::make_unique<MockTraderApi>();
}

}  // namespace qtrade::adapter::mock::trader
