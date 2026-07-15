/// @file      execution_manager.hpp
/// @brief     交易执行管理器
/// @details   负责订单执行、报单管理以及成交回报处理
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_EXECUTION_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_EXECUTION_MANAGER_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_sdk/trader/trader_api.hpp>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace qtrade::engine::ems {

class ExecutionManager {
 public:
  ~ExecutionManager();

  void Start();
  void Stop();
  void SetTraderApi(qtrade_sdk::trader::TraderApi* trader_api);
  ErrorCode Enqueue(const qtrade_sdk::trader::Order& order);

 private:
  void Run();

  qtrade_sdk::trader::TraderApi* trader_api_ = nullptr;
  std::deque<qtrade_sdk::trader::Order> pending_orders_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::thread worker_;
  bool running_ = false;
};

}  // namespace qtrade::engine::ems

#endif  // QTRADE_TRADING_ENGINE_EXECUTION_MANAGER_HPP_
