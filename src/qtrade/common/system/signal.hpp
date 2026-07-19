/// @file      signal.hpp
/// @brief     信号处理工具（参考 ugos_serv common/system/signal）
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_SYSTEM_SIGNAL_HPP_
#define QTRADE_COMMON_SYSTEM_SIGNAL_HPP_

namespace qtrade::common::system {

/// @brief 阻塞 SIGINT/SIGTERM（建议在启动最早期调用）
/// @details 阻塞后后续创建的线程会继承该 mask；主路径用 WaitInterruptSignals() 等待停机
void BlockInterruptSignals();

/// @brief 等待 SIGINT/SIGTERM（阻塞，基于 sigwait）
/// @return 收到的信号编号；失败返回 -1
/// @note 调用前须先 BlockInterruptSignals()
/// @return 收到的信号编号；失败返回 -1
[[nodiscard]] int WaitInterruptSignals();

}  // namespace qtrade::common::system

#endif  // QTRADE_COMMON_SYSTEM_SIGNAL_HPP_
