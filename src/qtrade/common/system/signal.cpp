/// @file      signal.cpp
/// @brief     信号处理工具实现
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/system/signal.hpp"

#include <spdlog/spdlog.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <pthread.h>

namespace qtrade::common::system {

void BlockInterruptSignals() {
  sigset_t set;
  ::sigemptyset(&set);
  ::sigaddset(&set, SIGINT);
  ::sigaddset(&set, SIGTERM);
  if (::pthread_sigmask(SIG_BLOCK, &set, nullptr) != 0) {
    spdlog::error("[system::signal] Failed to block termination signals: {}", std::strerror(errno));
  }
}

int WaitInterruptSignals() {
  sigset_t set;
  ::sigemptyset(&set);
  ::sigaddset(&set, SIGINT);
  ::sigaddset(&set, SIGTERM);

  int sig = 0;
  if (::sigwait(&set, &sig) != 0) {
    spdlog::error("[system::signal] sigwait failed: {}", std::strerror(errno));
    return -1;
  }
  return sig;
}

}  // namespace qtrade::common::system
