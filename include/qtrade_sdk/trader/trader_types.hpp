/// @file      trader_types.hpp
/// @brief     交易模块枚举定义
/// @details   参考 EMT_API 交易侧类型（EMT_MARKET_TYPE / EMT_SIDE_TYPE / EMT_FUND_TRANSFER_TYPE 等）。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SDK_TRADER_TYPES_HPP_
#define QTRADE_SDK_TRADER_TYPES_HPP_

#include <cstdint>

namespace qtrade_sdk::trader {

/// @brief 通讯传输协议类型。
enum class ProtocolType : std::uint8_t {
  /// TCP 协议。
  kTcp = 1,
  /// UDP 协议。
  kUdp = 2,
  /// 未指定或未知协议。
  kUnknown = 0,
};

/// @brief 交易侧市场类型。
enum class MarketType : std::uint8_t {
  /// 未初始化。
  kInit = 0,
  /// 上海 A 股市场。
  kShA = 1,
  /// 深圳 A 股市场。
  kSzA = 2,
  /// 沪港通市场。
  kShHkConnect = 3,
  /// 深港通市场。
  kSzHkConnect = 4,
  /// 北京 A 股市场。
  kBjA = 5,
  /// 未知市场。
  kUnknown = 99,
};

/// @brief 委托价格类型。
enum class PriceType : std::uint8_t {
  /// 限价。
  kLimit = 1,
  /// 即时成交剩余撤销。
  kBestOrCancel = 2,
  /// 最优五档即时成交剩余限价。
  kBest5OrLimit = 3,
  /// 最优五档即时成交剩余撤销。
  kBest5OrCancel = 4,
  /// 全部成交或撤销。
  kAllOrCancel = 5,
  /// 本方最优。
  kForwardBest = 6,
  /// 对方最优剩余限价。
  kReverseBestLimit = 7,
  /// 即时成交剩余限价。
  kLimitOrCancel = 8,
  /// 港股竞价限价。
  kHkLimitBidding = 9,
  /// 港股增强限价。
  kHkLimitEnhanced = 10,
  /// 未知价格类型。
  kUnknown = 255,
};

/// @brief 买卖方向；对应 EMT 中 EMT_SIDE_TYPE 的取值语义。
enum class SideType : std::uint8_t {
  /// 买入，对应 EMT_SIDE_BUY。
  kBuy = 1,
  /// 卖出，对应 EMT_SIDE_SELL。
  kSell = 2,
  /// 申购，对应 EMT_SIDE_PURCHASE。
  kPurchase = 7,
  /// 赎回，对应 EMT_SIDE_REDEMPTION。
  kRedemption = 8,
  /// 拆分，对应 EMT_SIDE_SPLIT。
  kSplit = 9,
  /// 合并，对应 EMT_SIDE_MERGE。
  kMerge = 10,
  /// 备兑，对应 EMT_SIDE_COVER。
  kCover = 11,
  /// 融资买入，对应 EMT_SIDE_MARGIN_TRADE。
  kMarginTrade = 21,
  /// 融券卖出，对应 EMT_SIDE_SHORT_SELL。
  kShortSell = 22,
  /// 卖券还款，对应 EMT_SIDE_REPAY_MARGIN。
  kRepayMargin = 23,
  /// 买券还券，对应 EMT_SIDE_REPAY_STOCK。
  kRepayStock = 24,
  /// 未知或无效买卖方向，对应 EMT_SIDE_UNKNOWN。
  kUnknown = 50,
};

/// @brief 开平标识。
enum class PositionEffectType : std::uint8_t {
  /// 初始值；现货业务通常填此值。
  kInit = 0,
  /// 开仓。
  kOpen = 1,
  /// 平仓。
  kClose = 2,
  /// 强平。
  kForceClose = 3,
  /// 平今。
  kCloseToday = 4,
  /// 平昨。
  kCloseYesterday = 5,
  /// 强减。
  kForceOff = 6,
  /// 本地强平。
  kLocalForceClose = 7,
  /// 未知开平标识。
  kUnknown = 12,
};

/// @brief 业务类型。
enum class BusinessType : std::uint8_t {
  /// 普通股票业务。
  kCash = 0,
  /// 新股申购业务。
  kIpo = 1,
  /// 回购业务。
  kRepo = 2,
  /// ETF 申赎业务。
  kEtf = 3,
  /// 融资融券业务。
  kMargin = 4,
  /// 配股业务。
  kAllotment = 6,
  /// 期权业务。
  kOption = 10,
  /// 行权业务。
  kExecute = 11,
  /// 锁定解锁业务。
  kFreeze = 12,
  /// 盘后固定定价交易。
  kFixPrice = 14,
  /// 未知业务类型。
  kUnknown = 255,
};

/// @brief 报单状态。
enum class OrderStatusType : std::uint8_t {
  /// 初始化状态。
  kInit = 0,
  /// SDK 兼容别名：新订单/初始状态。
  kNew = 0,
  /// 全部成交。
  kAllTraded = 1,
  /// SDK 兼容别名：全部成交。
  kFilled = 1,
  /// 部分成交仍在队列中。
  kPartTradedQueueing = 2,
  /// SDK 兼容别名：部分成交仍在队列中。
  kPartiallyFilled = 2,
  /// 部分成交不在队列中。
  kPartTradedNotQueueing = 3,
  /// SDK 兼容别名：部分成交不在队列中。
  kPartiallyFilledNotQueueing = 3,
  /// 未成交仍在队列中。
  kNoTradeQueueing = 4,
  /// SDK 兼容别名：未成交仍在队列中。
  kNotTradedQueueing = 4,
  /// 未成交不在队列中。
  kNoTradeNotQueueing = 5,
  /// 已撤单。
  kCanceled = 6,
  /// 已拒绝。
  kRejected = 7,
  /// 未知报单状态。
  kUnknown = 255,
};

/// @brief 报单提交状态。
enum class OrderSubmitStatusType : std::uint8_t {
  /// 初始状态。
  kInit = 0,
  /// 已提交报单。
  kInsertSubmitted = 1,
  /// 报单已被接受。
  kInsertAccepted = 2,
  /// 报单已被拒绝。
  kInsertRejected = 3,
  /// 已提交撤单。
  kCancelSubmitted = 4,
  /// 撤单已被拒绝。
  kCancelRejected = 5,
  /// 撤单已被接受。
  kCancelAccepted = 6,
};

/// @brief 持仓方向。
enum class PositionDirectionType : std::uint8_t {
  /// 未知方向。
  kUnknown = 0,
  /// 多头持仓。
  kLong = 1,
  /// 空头持仓。
  kShort = 2,
  /// 净持仓。
  kNet = 3,
};

/// @brief 账户类型。
enum class AccountType : std::uint8_t {
  /// 普通证券账户。
  kStock = 0,
  /// 信用账户。
  kCredit = 1,
  /// 期权账户。
  kOption = 2,
  /// 未知账户类型。
  kUnknown = 255,
};

/// @brief 成交类型。
enum class TradeType : std::uint8_t {
  /// 普通成交。
  kCommon = 0,
  /// 撤单回报。
  kCancel = 1,
  /// 未知成交类型。
  kUnknown = 255,
};

/// @brief 公共流重传方式。
enum class ResumeType : std::uint8_t {
  /// 从本交易日开始重传。
  kRestart = 0,
  /// 从上次收到的位置继续。
  kResume = 1,
  /// 只传送登录后公共流内容。
  kQuick = 2,
};

/// @brief 资金流转方向类型。
enum class FundTransferType : std::uint8_t {
  /// 从 EMT 转出到柜台。
  kOut = 0,
  /// 从柜台转入 EMT。
  kIn = 1,
  /// 跨节点转出。
  kInterOut = 2,
  /// 跨节点转入。
  kInterIn = 3,
  /// 未知类型。
  kUnknown = 255,
};

/// @brief 资金查询类型。
enum class FundQueryType : std::uint8_t {
  /// 查询主柜台可转资金。
  kJz = 0,
  /// 查询一账号两中心设置时对方节点资金。
  kInternalError = 1,
  /// 未知类型。
  kUnknown = 255
};

/// @brief 柜台资金操作结果。
enum class FundOperStatus : std::uint8_t {
  /// 处理中。
  kProcessing = 0,
  /// 成功。
  kSuccess = 1,
  /// 失败。
  kFailed = 2,
  /// 已提交。
  kSubmitted = 3,
  /// 未知状态。
  kUnknown = 255,
};

/// @brief 融资信用额度调拨方向类型。
enum class QuotaTransferType : std::uint8_t {
  /// 融券额度划出。
  kStkOut = 0,
  /// 融券额度划入。
  kStkIn = 1,
  /// 融资额度划出。
  kFundOut = 2,
  /// 融资额度划入。
  kFundIn = 3,
  /// 未知类型。
  kUnknown = 255
};

/// @brief 融资融券额度调拨操作结果。
enum class QuotaOperStatus : std::uint8_t {
  /// 处理中。
  kProcessing = 0,
  /// 成功。
  kSuccess = 1,
  /// 失败。
  kFailed = 2,
  /// 已提交。
  kSubmitted = 3,
  /// 未知状态。
  kUnknown = 255,
};

}  // namespace qtrade_sdk::trader

#endif  // QTRADE_SDK_TRADER_TYPES_HPP_
