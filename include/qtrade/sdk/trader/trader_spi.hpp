/// @file      trader_spi.hpp
/// @brief     交易回调接口
/// @details   参考 EMT::API::TraderSpi；业务数据使用 const 引用，可空字段保留指针。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SDK_TRADER_SPI_HPP_
#define QTRADE_SDK_TRADER_SPI_HPP_

#include <qtrade/sdk/trader/trader_struct.hpp>

#include <cstdint>

namespace qtrade_sdk::trader {

/// @brief 交易回调接口。
class TraderSpi {
 public:
  /// @brief 销毁交易回调接口实例。
  virtual ~TraderSpi() = default;

  /// @brief 连接成功通知。
  virtual void OnConnected() = 0;

  /// @brief 断线通知。
  /// @param reason 柜台返回的断线原因。
  virtual void OnDisconnected(int reason) = 0;

  /// @brief 错误通知。
  /// @param error_info 错误信息；为空表示无附加错误信息。
  virtual void OnError(const RspInfo* error_info) = 0;

  /// @brief 报单状态变化通知。
  /// @param order_info 报单回报信息。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param session_id 登录后取得的会话 ID。
  virtual void OnOrderEvent(const Order& order_info, const RspInfo* error_info, std::uint64_t session_id) = 0;

  /// @brief 成交通知。
  /// @param trade_info 成交回报信息。
  /// @param session_id 登录后取得的会话 ID。
  virtual void OnTradeEvent(const Trade& trade_info, std::uint64_t session_id) = 0;

  /// @brief 撤单错误响应。
  /// @param cancel_info 撤单错误详情。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param session_id 登录后取得的会话 ID。
  virtual void OnCancelOrderError(const OrderCancelInfo& cancel_info,
                                  const RspInfo* error_info,
                                  std::uint64_t session_id) = 0;

  /// @brief 查询委托响应。
  /// @param order_info 委托查询结果。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param request_id 与查询请求对应的请求编号。
  /// @param is_last 是否为本次查询最后一条回报。
  /// @param session_id 登录后取得的会话 ID。
  virtual void OnQueryOrder(
    const Order& order_info, const RspInfo* error_info, int request_id, bool is_last, std::uint64_t session_id) = 0;

  /// @brief 按页查询委托响应。
  /// @param order_info 委托查询结果。
  /// @param req_count 本次请求期望返回条数。
  /// @param order_sequence 当前回报序号。
  /// @param query_reference 分页查询引用。
  /// @param request_id 与查询请求对应的请求编号。
  /// @param is_last 是否为本次查询最后一条回报。
  /// @param session_id 登录后取得的会话 ID。
  virtual void OnQueryOrderByPage(const Order& order_info,
                                  std::int64_t req_count,
                                  std::int64_t order_sequence,
                                  std::int64_t query_reference,
                                  int request_id,
                                  bool is_last,
                                  std::uint64_t session_id) = 0;

  /// @brief 查询成交响应。
  /// @param trade_info 成交查询结果。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param request_id 与查询请求对应的请求编号。
  /// @param is_last 是否为本次查询最后一条回报。
  /// @param session_id 登录后取得的会话 ID。
  virtual void OnQueryTrade(
    const Trade& trade_info, const RspInfo* error_info, int request_id, bool is_last, std::uint64_t session_id) = 0;

  /// @brief 按页查询成交响应。
  /// @param trade_info 成交查询结果。
  /// @param req_count 本次请求期望返回条数。
  /// @param trade_sequence 当前回报序号。
  /// @param query_reference 分页查询引用。
  /// @param request_id 与查询请求对应的请求编号。
  /// @param is_last 是否为本次查询最后一条回报。
  /// @param session_id 登录后取得的会话 ID。
  virtual void OnQueryTradeByPage(const Trade& trade_info,
                                  std::int64_t req_count,
                                  std::int64_t trade_sequence,
                                  std::int64_t query_reference,
                                  int request_id,
                                  bool is_last,
                                  std::uint64_t session_id) = 0;

  /// @brief 查询持仓响应。
  /// @param position 持仓查询结果。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param request_id 与查询请求对应的请求编号。
  /// @param is_last 是否为本次查询最后一条回报。
  /// @param session_id 登录后取得的会话 ID。
  virtual void OnQueryPosition(
    const Position& position, const RspInfo* error_info, int request_id, bool is_last, std::uint64_t session_id) = 0;

  /// @brief 查询资金响应。
  /// @param asset 资金查询结果。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param request_id 与查询请求对应的请求编号。
  /// @param is_last 是否为本次查询最后一条回报。
  /// @param session_id 登录后取得的会话 ID。
  virtual void OnQueryAsset(
    const AccountAsset& asset, const RspInfo* error_info, int request_id, bool is_last, std::uint64_t session_id) = 0;

  /// @brief 查询其他节点资金响应。
  /// @param fund_info 其他节点资金查询结果。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param request_id 与查询请求对应的请求编号。
  /// @param session_id 登录后取得的会话 ID。
  virtual void OnQueryOtherServerFund(const FundQueryResponse& fund_info,
                                      const RspInfo* error_info,
                                      int request_id,
                                      std::uint64_t session_id) = 0;

  /// @brief 查询资金划拨流水响应。
  /// @param fund_transfer_info 资金划拨流水。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param request_id 与查询请求对应的请求编号。
  /// @param is_last 是否为本次查询最后一条回报。
  /// @param session_id 登录后取得的会话 ID。
  virtual void OnQueryFundTransfer(const FundTransferNotice& fund_transfer_info,
                                   const RspInfo* error_info,
                                   int request_id,
                                   bool is_last,
                                   std::uint64_t session_id) = 0;

  /// @brief 资金划拨通知。
  /// @param fund_transfer_info 资金划拨结果。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param session_id 登录后取得的会话 ID。
  virtual void OnFundTransfer(const FundTransferNotice& fund_transfer_info,
                              const RspInfo* error_info,
                              std::uint64_t session_id) = 0;

  /// @brief 信用额度调拨通知。
  /// @param quota_transfer_info 额度调拨结果。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param session_id 登录后取得的会话 ID。
  virtual void OnCreditQuotaTransfer(const QuotaTransferNotice& quota_transfer_info,
                                     const RspInfo* error_info,
                                     std::uint64_t session_id) = 0;

  /// @brief 查询信用额度调拨流水响应。
  /// @param quota_transfer_info 额度调拨流水。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param request_id 与查询请求对应的请求编号。
  /// @param is_last 是否为本次查询最后一条回报。
  /// @param session_id 登录后取得的会话 ID。
  virtual void OnQueryCreditQuotaTransfer(const QuotaTransferNotice& quota_transfer_info,
                                          const RspInfo* error_info,
                                          int request_id,
                                          bool is_last,
                                          std::uint64_t session_id) = 0;
};

}  // namespace qtrade_sdk::trader

#endif  // QTRADE_SDK_TRADER_SPI_HPP_
