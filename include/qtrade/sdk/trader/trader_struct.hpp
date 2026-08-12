/// @file      trader_struct.hpp
/// @brief     交易相关结构体
/// @details   参考 EMT_API 交易结构体，聚合登录、报单、撤单、查询、资金、持仓与成交回报。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SDK_TRADER_STRUCT_HPP_
#define QTRADE_SDK_TRADER_STRUCT_HPP_

#include <qtrade/sdk/trader/fund_struct.hpp>
#include <qtrade/sdk/trader/trader_types.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace qtrade::sdk::trader {

/// @brief 交易 API 错误响应信息，对应 EMTRI。
struct RspInfo {
  /// 错误代码；0 表示无错误。
  std::int32_t error_id = 0;
  /// 错误信息。
  std::string error_msg;
};

/// @brief 用户终端信息，对应 EMTUserTerminalInfoReq 的必要字段。
struct UserTerminalInfo {
  /// 本机 IP 地址。
  std::string local_ip;
  /// 本机 MAC 地址。
  std::string mac_address;
  /// 硬盘序列号。
  std::string harddisk_sn;
  /// 终端类型或厂商自定义标识。
  std::string terminal_type;
  /// 软件名称。
  std::string software_name;
  /// 软件版本。
  std::string software_version;
};

/// @brief 连接交易网关请求。
struct ConnectRequest {
  /// 券商或通道 ID。
  std::string broker_id;
  /// 连接串，如 tcp://host:port。
  std::string connection_string;
  /// 资金账户。
  std::string account_id;
  /// 密码或 token。
  std::string password;
  /// 通讯协议。
  ProtocolType protocol = ProtocolType::kTcp;
  /// 本地绑定 IP，可为空。
  std::string local_ip;
  /// 终端采集信息。
  UserTerminalInfo terminal_info;
};

/// @brief 报单录入信息，对应 EMTOrderInsertInfo 的核心字段。
struct OrderRequest {
  /// 用户自定义报单引用。
  std::uint32_t client_order_id = 0;
  /// 券商/通道委托号；发单前通常为 0，由适配器在回报中回填（EMT 对应 order_emt_id）。
  std::uint64_t broker_order_id = 0;
  /// 合约代码。
  std::string instrument;
  /// 交易市场。
  MarketType market = MarketType::kUnknown;
  /// 委托价格，单位元。
  double price = 0.0;
  /// 止损价，当前保留。
  double stop_price = 0.0;
  /// 委托数量，单位股/张。
  std::int64_t volume = 0;
  /// 委托价格类型。
  PriceType price_type = PriceType::kLimit;
  /// 买卖方向。
  SideType side = SideType::kBuy;
  /// 开平标识，现货通常为 kInit。
  PositionEffectType position_effect = PositionEffectType::kInit;
  /// 业务类型。
  BusinessType business_type = BusinessType::kCash;
};

/// @brief 撤单请求。
struct CancelOrderRequest {
  /// SDK 内部订单 ID。
  std::string order_id;
  /// 待撤订单的券商/通道委托号。
  std::uint64_t broker_order_id = 0;
  /// 登录后取得的会话 ID。
  std::uint64_t session_id = 0;
};

/// @brief 撤单错误信息，对应 EMTOrderCancelInfo。
struct OrderCancelInfo {
  /// 撤单请求在券商/通道侧的 ID。
  std::uint64_t broker_cancel_id = 0;
  /// 原委托的券商/通道委托号。
  std::uint64_t broker_order_id = 0;
  /// SDK 内部订单 ID。
  std::string order_id;
};

/// @brief 报单回报，对应 EMTOrderInfo/EMTQueryOrderRsp 的核心字段。
struct Order {
  /// SDK 内部订单 ID。
  std::string order_id;
  /// 券商/通道委托号。
  std::uint64_t broker_order_id = 0;
  /// 用户自定义报单引用。
  std::uint32_t client_order_id = 0;
  /// 交易所报单编号。
  std::string exchange_order_id;
  /// 合约代码。
  std::string instrument;
  /// 交易市场。
  MarketType market = MarketType::kUnknown;

  /// 委托价格，单位元。
  double price = 0.0;
  /// 委托数量。
  std::int64_t volume = 0;
  /// 累计成交数量。
  std::int64_t traded_volume = 0;
  /// 剩余数量。
  std::int64_t left_volume = 0;
  /// 累计成交金额。
  double trade_amount = 0.0;

  /// 价格类型。
  PriceType price_type = PriceType::kLimit;
  /// 买卖方向。
  SideType side = SideType::kBuy;
  /// 开平标识。
  PositionEffectType position_effect = PositionEffectType::kInit;
  /// 业务类型。
  BusinessType business_type = BusinessType::kCash;

  /// 报单状态。
  OrderStatusType status = OrderStatusType::kInit;
  /// 报单提交状态。
  OrderSubmitStatusType submit_status = OrderSubmitStatusType::kInit;

  /// 委托时间，格式 YYYYMMDDHHMMSSsss。
  std::int64_t insert_time = 0;
  /// 最后更新时间。
  std::int64_t update_time = 0;
  /// 撤销时间。
  std::int64_t cancel_time = 0;

  /// 拒单或错误信息。
  std::string error_message;
};

/// @brief 成交回报，对应 EMTTradeReport/EMTQueryTradeRsp 的核心字段。
struct Trade {
  /// 成交编号。
  std::string trade_id;
  /// SDK 内部订单 ID。
  std::string order_id;
  /// 券商/通道委托号。
  std::uint64_t broker_order_id = 0;
  /// 用户自定义报单引用。
  std::uint32_t client_order_id = 0;
  /// 交易所报单编号。
  std::string exchange_order_id;
  /// 合约代码。
  std::string instrument;
  /// 交易市场。
  MarketType market = MarketType::kUnknown;

  /// 成交价格，单位元。
  double price = 0.0;
  /// 本次成交数量。
  std::int64_t volume = 0;
  /// 本次成交金额。
  double trade_amount = 0.0;
  /// 成交时间，格式 YYYYMMDDHHMMSSsss。
  std::int64_t trade_time = 0;

  /// 买卖方向。
  SideType side = SideType::kBuy;
  /// 开平标识。
  PositionEffectType position_effect = PositionEffectType::kInit;
  /// 成交类型。
  TradeType trade_type = TradeType::kCommon;
  /// 业务类型。
  BusinessType business_type = BusinessType::kCash;

  /// 回报序号。
  std::uint64_t report_index = 0;
};

/// @brief 持仓查询结果，对应 EMTQueryStkPositionRsp 的核心字段。
struct Position {
  /// 证券代码。
  std::string instrument;
  /// 证券名称。
  std::string instrument_name;
  /// 交易市场。
  MarketType market = MarketType::kUnknown;

  /// 总持仓。
  std::int64_t total_volume = 0;
  /// 可卖持仓。
  std::int64_t sellable_volume = 0;
  /// 昨日持仓。
  std::int64_t yesterday_volume = 0;
  /// 今日申购/赎回数量。
  std::int64_t purchase_redeemable_volume = 0;

  /// 持仓成本价。
  double avg_price = 0.0;
  /// 浮动盈亏。
  double unrealized_pnl = 0.0;

  /// 持仓方向。
  PositionDirectionType direction = PositionDirectionType::kLong;

  /// 可行权合约数量，期权使用。
  std::int64_t executable_option = 0;
  /// 可锁定标的数量。
  std::int64_t lockable_volume = 0;
  /// 可行权标的数量。
  std::int64_t executable_underlying = 0;
  /// 已锁定标的数量。
  std::int64_t locked_volume = 0;
  /// 可用已锁定标的数量。
  std::int64_t usable_locked_volume = 0;
};

/// @brief 账户资金查询结果，对应 EMTQueryAssetRsp 的核心字段。
struct AccountAsset {
  /// 资金账户 ID。
  std::string account_id;
  /// 账户类型。
  AccountType account_type = AccountType::kStock;

  /// 总资产，单位元。
  double total_asset = 0.0;
  /// 可用资金，单位元。
  double buying_power = 0.0;
  /// 证券资产，单位元。
  double security_asset = 0.0;
  /// 预扣资金，单位元。
  double withholding_amount = 0.0;

  /// 累计买入占用，单位元。
  double fund_buy_amount = 0.0;
  /// 累计买入费用，单位元。
  double fund_buy_fee = 0.0;
  /// 累计卖出所得，单位元。
  double fund_sell_amount = 0.0;
  /// 累计卖出费用，单位元。
  double fund_sell_fee = 0.0;

  /// 冻结保证金，期权使用。
  double frozen_margin = 0.0;
  /// 行权冻结资金，期权使用。
  double frozen_exec_cash = 0.0;
  /// 可取资金，期权使用。
  double preferred_amount = 0.0;

  /// 融券卖出所得可用余额。
  double repay_stock_available_balance = 0.0;
  /// 港股通可用资金。
  double hkex_fund_available = 0.0;
  /// 港股通冻结资金。
  double hkex_fund_frozen = 0.0;
};

/// @brief 查询委托请求。
struct QueryOrdersRequest {
  /// 合约过滤，空表示全部。
  std::string instrument;
  /// 状态过滤。
  OrderStatusType status = OrderStatusType::kUnknown;
  /// 指定券商/通道委托号，0 表示不限定。
  std::uint64_t broker_order_id = 0;
};

/// @brief 按页查询委托请求。
struct QueryOrdersByPageRequest {
  /// 本次请求最大返回数量。
  std::int64_t req_count = 0;
  /// 查询引用，首次查询填 0。
  std::int64_t reference = 0;
  /// 保留字段。
  std::int64_t reserved = 0;
};

/// @brief 查询成交请求。
struct QueryTradesRequest {
  /// 合约过滤，空表示全部。
  std::string instrument;
  /// SDK 订单 ID 过滤，空表示全部。
  std::string order_id;
  /// 券商/通道委托号过滤，0 表示全部。
  std::uint64_t broker_order_id = 0;
};

/// @brief 查询持仓请求。
struct QueryPositionRequest {
  /// 合约过滤，空表示全部。
  std::string instrument;
  /// 市场过滤。
  MarketType market = MarketType::kInit;
};

/// @brief 查询资金请求。
struct QueryAssetRequest {
  /// 资金账户。
  std::string account_id;
};

/// @brief 委托列表响应。
struct QueryOrdersResponse {
  /// 委托列表。
  std::vector<Order> orders;
};

/// @brief 成交列表响应。
struct QueryTradesResponse {
  /// 成交列表。
  std::vector<Trade> trades;
};

/// @brief 持仓列表响应。
struct QueryPositionResponse {
  /// 持仓列表。
  std::vector<Position> positions;
};

/// @brief 资金查询响应。
struct QueryAssetResponse {
  /// 账户资金信息。
  AccountAsset asset;
};

}  // namespace qtrade::sdk::trader

#endif  // QTRADE_SDK_TRADER_STRUCT_HPP_
