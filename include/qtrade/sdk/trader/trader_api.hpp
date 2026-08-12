/// @file      trader_api.hpp
/// @brief     交易 API 抽象接口
/// @details   参考 EMT::API::TraderApi，所有接口均为纯虚函数。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SDK_TRADER_API_HPP_
#define QTRADE_SDK_TRADER_API_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/trader/trader_spi.hpp>

#include <cstdint>
#include <functional>
#include <string>

namespace qtrade::sdk::trader {

/// @brief 交易 API 抽象接口。
class TraderApi {
 public:
  /// @brief 订单回报回调函数类型。
  using OrderCallback = std::function<void(const Order&)>;
  /// @brief 成交回报回调函数类型。
  using TradeCallback = std::function<void(const Trade&)>;

  /// @brief 销毁交易 API 实例。
  virtual ~TraderApi() = default;

  /// @brief 设置 API 工作线程 CPU 亲和。
  /// @param thread1_cpu_core_id 工作线程 1 绑定 CPU；-1 表示不绑核。
  /// @param thread2_cpu_core_id 工作线程 2 绑定 CPU；-1 表示不绑核。
  virtual void SetCpuAffinity(std::int32_t thread1_cpu_core_id = -1, std::int32_t thread2_cpu_core_id = -1) = 0;

  /// @brief 根据错误码获取错误说明。
  /// @param error_code 错误代码。
  /// @param session_id 登录后取得的会话 ID。
  /// @return 错误说明文本。
  virtual std::string GetErrMsgByCode(std::int32_t error_code, std::uint64_t session_id) = 0;

  /// @brief 释放 API 实例资源。
  virtual void Release() = 0;

  /// @brief 获取当前交易日。
  /// @return 交易日字符串，格式 YYYYMMDD。
  virtual std::string GetTradingDay() const = 0;

  /// @brief 注册交易回调接口；应在登录前调用。
  /// @param spi 交易 SPI 实现对象。
  virtual void RegisterSpi(TraderSpi& spi) = 0;

  /// @brief 取消注册交易回调接口。
  virtual void UnregisterSpi() = 0;

  /// @brief 获取 API 最近一次错误。
  /// @return 错误信息指针；无错误时返回 nullptr。
  virtual const RspInfo* GetApiLastError() const = 0;

  /// @brief 获取 API 版本。
  /// @return API 版本字符串。
  virtual std::string GetApiVersion() const = 0;

  /// @brief 通过券商/通道委托号获取 client_id。
  /// @param broker_order_id 券商/通道委托号。
  /// @return 客户端 ID。
  virtual std::uint8_t GetClientIDByBrokerOrderId(std::uint64_t broker_order_id) = 0;

  /// @brief 通过券商/通道委托号获取资金账号。
  /// @param broker_order_id 券商/通道委托号。
  /// @return 资金账号字符串。
  virtual std::string GetAccountByBrokerOrderId(std::uint64_t broker_order_id) = 0;

  /// @brief 订阅公共流重传方式。
  /// @param resume_type 公共流重传方式。
  virtual void SubscribePublicTopic(ResumeType resume_type) = 0;

  /// @brief 设置客户端软件版本。
  /// @param version 客户端软件版本号。
  virtual void SetSoftwareVersion(const std::string& version) = 0;

  /// @brief 设置心跳间隔。
  /// @param interval 心跳间隔，单位秒。
  virtual void SetHeartBeatInterval(std::uint32_t interval) = 0;

  /// @brief 项目内部交易网关连接入口。
  /// @param request 连接请求参数。
  /// @return 连接结果错误码。
  virtual qtrade::ErrorCode Connect(const ConnectRequest& request) = 0;

  /// @brief 项目内部交易网关断开入口。
  virtual void Disconnect() = 0;

  /// @brief 返回当前连接状态。
  /// @return 已连接返回 true，否则返回 false。
  virtual bool IsConnected() const = 0;

  /// @brief 用户登录请求。
  /// @param ip 交易服务器地址。
  /// @param port 交易服务器端口。
  /// @param user 登录用户名。
  /// @param password 登录密码。
  /// @param sock_type 通讯协议类型。
  /// @param local_ip 本地绑定 IP，可为空。
  /// @param terminal_info 终端采集信息，可为空。
  /// @return 登录成功返回 session_id；0 表示登录失败。
  virtual std::uint64_t Login(const std::string& ip,
                              int port,
                              const std::string& user,
                              const std::string& password,
                              ProtocolType sock_type,
                              const std::string& local_ip = {},
                              const UserTerminalInfo* terminal_info = nullptr) = 0;

  /// @brief 用户登出请求。
  /// @param session_id 登录后取得的会话 ID。
  /// @return 0 表示成功，非 0 表示失败。
  virtual int Logout(std::uint64_t session_id) = 0;

  /// @brief 判断交易后台是否重启。
  /// @param session_id 登录后取得的会话 ID。
  /// @return 后台已重启返回 true，否则返回 false。
  virtual bool IsServerRestart(std::uint64_t session_id) = 0;

  /// @brief 报单录入请求。
  /// @param order 报单录入信息。
  /// @param session_id 登录后取得的会话 ID。
  /// @return 券商/通道委托号；0 表示发送失败。
  virtual std::uint64_t InsertOrder(const OrderRequest& order, std::uint64_t session_id) = 0;

  /// @brief 项目内部统一发单入口。
  /// @param request 报单请求。
  /// @return 发单结果错误码。
  virtual qtrade::ErrorCode SendOrder(const OrderRequest& request) = 0;

  /// @brief 撤单请求。
  /// @param broker_order_id 待撤订单的券商/通道委托号。
  /// @param session_id 登录后取得的会话 ID。
  /// @return 券商/通道撤单号；0 表示发送失败。
  virtual std::uint64_t CancelOrder(std::uint64_t broker_order_id, std::uint64_t session_id) = 0;

  /// @brief 项目内部统一撤单入口。
  /// @param request 撤单请求。
  /// @return 撤单结果错误码。
  virtual qtrade::ErrorCode CancelOrder(const CancelOrderRequest& request) = 0;

  /// @brief 根据券商/通道委托号查询委托。
  /// @param broker_order_id 待查询订单的券商/通道委托号。
  /// @param session_id 登录后取得的会话 ID。
  /// @param request_id 用户自定义请求编号。
  /// @return 0 表示查询请求发送成功，非 0 表示失败。
  virtual int QueryOrderByBrokerOrderId(std::uint64_t broker_order_id, std::uint64_t session_id, int request_id) = 0;

  /// @brief 查询委托。
  /// @param query_param 查询过滤条件。
  /// @param session_id 登录后取得的会话 ID。
  /// @param request_id 用户自定义请求编号。
  /// @return 0 表示查询请求发送成功，非 0 表示失败。
  virtual int QueryOrders(const QueryOrdersRequest& query_param, std::uint64_t session_id, int request_id) = 0;

  /// @brief 项目内部统一委托查询入口。
  /// @param request 查询过滤条件。
  /// @param response 查询结果输出。
  /// @return 查询结果错误码。
  virtual qtrade::ErrorCode QueryOrders(const QueryOrdersRequest& request, QueryOrdersResponse& response) = 0;

  /// @brief 查询未完结委托。
  /// @param session_id 登录后取得的会话 ID。
  /// @param request_id 用户自定义请求编号。
  /// @return 0 表示查询请求发送成功，非 0 表示失败。
  virtual int QueryUnfinishedOrders(std::uint64_t session_id, int request_id) = 0;

  /// @brief 按页查询委托。
  /// @param query_param 分页查询条件。
  /// @param session_id 登录后取得的会话 ID。
  /// @param request_id 用户自定义请求编号。
  /// @return 0 表示查询请求发送成功，非 0 表示失败。
  virtual int QueryOrdersByPage(const QueryOrdersByPageRequest& query_param,
                                std::uint64_t session_id,
                                int request_id) = 0;

  /// @brief 根据券商/通道委托号查询成交。
  /// @param broker_order_id 待查询订单的券商/通道委托号。
  /// @param session_id 登录后取得的会话 ID。
  /// @param request_id 用户自定义请求编号。
  /// @return 0 表示查询请求发送成功，非 0 表示失败。
  virtual int QueryTradesByBrokerOrderId(std::uint64_t broker_order_id, std::uint64_t session_id, int request_id) = 0;

  /// @brief 查询成交。
  /// @param query_param 查询过滤条件。
  /// @param session_id 登录后取得的会话 ID。
  /// @param request_id 用户自定义请求编号。
  /// @return 0 表示查询请求发送成功，非 0 表示失败。
  virtual int QueryTrades(const QueryTradesRequest& query_param, std::uint64_t session_id, int request_id) = 0;

  /// @brief 项目内部统一成交查询入口。
  /// @param request 查询过滤条件。
  /// @param response 查询结果输出。
  /// @return 查询结果错误码。
  virtual qtrade::ErrorCode QueryTrades(const QueryTradesRequest& request, QueryTradesResponse& response) = 0;

  /// @brief 查询持仓。
  /// @param ticker 证券代码；空字符串表示查询全部。
  /// @param session_id 登录后取得的会话 ID。
  /// @param request_id 用户自定义请求编号。
  /// @param market 交易市场过滤条件。
  /// @return 0 表示查询请求发送成功，非 0 表示失败。
  virtual int QueryPosition(const std::string& ticker,
                            std::uint64_t session_id,
                            int request_id,
                            MarketType market = MarketType::kInit) = 0;

  /// @brief 项目内部统一持仓查询入口。
  /// @param request 查询过滤条件。
  /// @param response 查询结果输出。
  /// @return 查询结果错误码。
  virtual qtrade::ErrorCode QueryPositions(const QueryPositionRequest& request, QueryPositionResponse& response) = 0;

  /// @brief 查询资金。
  /// @param session_id 登录后取得的会话 ID。
  /// @param request_id 用户自定义请求编号。
  /// @return 0 表示查询请求发送成功，非 0 表示失败。
  virtual int QueryAsset(std::uint64_t session_id, int request_id) = 0;

  /// @brief 项目内部统一资金查询入口。
  /// @param request 查询过滤条件。
  /// @param response 查询结果输出。
  /// @return 查询结果错误码。
  virtual qtrade::ErrorCode QueryAsset(const QueryAssetRequest& request, QueryAssetResponse& response) = 0;

  /// @brief 资金划拨请求。
  /// @param fund_transfer 资金划拨请求。
  /// @param session_id 登录后取得的会话 ID。
  /// @return 资金划拨流水 ID；0 表示发送失败。
  virtual std::uint64_t FundTransfer(const FundTransferRequest& fund_transfer, std::uint64_t session_id) = 0;

  /// @brief 查询其他节点资金。
  /// @param query_param 资金查询条件。
  /// @param session_id 登录后取得的会话 ID。
  /// @param request_id 用户自定义请求编号。
  /// @return 0 表示查询请求发送成功，非 0 表示失败。
  virtual int QueryOtherServerFund(const FundQueryRequest& query_param, std::uint64_t session_id, int request_id) = 0;

  /// @brief 信用额度调拨请求。
  /// @param quota_transfer 额度调拨请求。
  /// @param session_id 登录后取得的会话 ID。
  /// @return 额度调拨流水 ID；0 表示发送失败。
  virtual std::uint64_t CreditQuotaTransfer(const QuotaTransferRequest& quota_transfer, std::uint64_t session_id) = 0;

  /// @brief 查询信用额度调拨流水。
  /// @param query_param 查询条件。
  /// @param session_id 登录后取得的会话 ID。
  /// @param request_id 用户自定义请求编号。
  /// @return 0 表示查询请求发送成功，非 0 表示失败。
  virtual int QueryCreditQuotaTransfer(const QueryQuotaTransferLogRequest& query_param,
                                       std::uint64_t session_id,
                                       int request_id) = 0;

  /// @brief 查询资金划拨流水。
  /// @param query_param 查询条件。
  /// @param session_id 登录后取得的会话 ID。
  /// @param request_id 用户自定义请求编号。
  /// @return 0 表示查询请求发送成功，非 0 表示失败。
  virtual int QueryFundTransfer(const QueryFundTransferLogRequest& query_param,
                                std::uint64_t session_id,
                                int request_id) = 0;

  /// @brief 注册订单回报回调。
  /// @param cb 订单回报回调函数。
  virtual void SetOrderCallback(OrderCallback cb) = 0;

  /// @brief 注册成交回报回调。
  /// @param cb 成交回报回调函数。
  virtual void SetTradeCallback(TradeCallback cb) = 0;
};

/// @brief 交易网关接口别名。
using ITradeGateway = TraderApi;
/// @brief 执行适配器接口别名。
using IExecutionAdapter = TraderApi;

}  // namespace qtrade::sdk::trader

#endif  // QTRADE_SDK_TRADER_API_HPP_
