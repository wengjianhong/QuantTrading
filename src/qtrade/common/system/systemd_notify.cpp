/// @file      systemd_notify.cpp
/// @brief     systemd 服务状态通知实现（经 NOTIFY_SOCKET 发送，无硬依赖 libsystemd）
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/system/systemd_notify.hpp"

#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

namespace qtrade::common::system {
namespace {

/// @brief 向 $NOTIFY_SOCKET 发送一行或多行状态；未设置环境变量则跳过
[[nodiscard]] int SdNotify(const std::string& state) {
  if (state.empty()) {
    return 0;
  }

  const char* socket_path = ::getenv("NOTIFY_SOCKET");
  if (socket_path == nullptr || socket_path[0] == '\0') {
    return 0;
  }

  // abstract namespace: '@' → '\0'
  sockaddr_un address{};
  address.sun_family = AF_UNIX;
  std::string path = socket_path;
  if (path[0] == '@') {
    path[0] = '\0';
  }
  if (path.size() >= sizeof(address.sun_path)) {
    spdlog::error("[system::systemd_notify] NOTIFY_SOCKET path too long");
    return -1;
  }
  std::memcpy(address.sun_path, path.data(), path.size());

  const int fd = ::socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
  if (fd < 0) {
    spdlog::error("[system::systemd_notify] socket failed: {}", std::strerror(errno));
    return -1;
  }

  const socklen_t addr_len = static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) + path.size());
  if (::connect(fd, reinterpret_cast<sockaddr*>(&address), addr_len) != 0) {
    spdlog::error("[system::systemd_notify] connect failed: {}", std::strerror(errno));
    ::close(fd);
    return -1;
  }

  const ssize_t written = ::write(fd, state.data(), state.size());
  ::close(fd);
  if (written < 0 || static_cast<size_t>(written) != state.size()) {
    spdlog::error("[system::systemd_notify] write failed: {}", std::strerror(errno));
    return -1;
  }
  return 1;
}

}  // namespace

bool NotifyStatus(const std::string& status) {
  if (status.empty()) {
    return true;
  }
  const std::string msg = "STATUS=" + status;
  const int ret = SdNotify(msg);
  spdlog::info("[system::systemd_notify] STATUS={}, ret={}", status, ret);
  return ret >= 0;
}

bool NotifyReady(const std::string& status) {
  std::string msg = "READY=1";
  if (!status.empty()) {
    msg += "\nSTATUS=" + status;
  }
  const int ret = SdNotify(msg);
  spdlog::info("[system::systemd_notify] READY=1 status='{}', ret={}", status, ret);
  return ret >= 0;
}

bool NotifyStopping(const std::string& status) {
  std::string msg = "STOPPING=1";
  if (!status.empty()) {
    msg += "\nSTATUS=" + status;
  }
  const int ret = SdNotify(msg);
  spdlog::info("[system::systemd_notify] STOPPING=1 status='{}', ret={}", status, ret);
  return ret >= 0;
}

bool NotifyError(int errno_value, const std::string& error_msg) {
  if (errno_value == 0 && error_msg.empty()) {
    return true;
  }

  std::string msg;
  if (errno_value != 0) {
    msg += "ERRNO=" + std::to_string(errno_value);
  }
  if (!error_msg.empty()) {
    if (!msg.empty()) {
      msg += "\n";
    }
    msg += "STATUS=" + error_msg;
  }

  const int ret = SdNotify(msg);
  spdlog::info("[system::systemd_notify] {}, ret={}", msg, ret);
  return ret >= 0;
}

}  // namespace qtrade::common::system
