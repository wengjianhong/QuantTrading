/// @file      engine_fence.cpp
/// @brief     单实例写入围栏与 engine_epoch 分配实现
/// @details   使用 flock(LOCK_EX|LOCK_NB) 抢占共享围栏文件，并原子写入新 epoch
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/engine_fence.hpp"

#include <sys/file.h>

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <unistd.h>

namespace qtrade::engine {

EngineFence::~EngineFence() {
  Release();
}

ErrorCode EngineFence::Acquire(const std::string& path, std::uint64_t minimum_epoch) {
  // 1. 校验参数并确保父目录存在
  if (path.empty() || minimum_epoch == 0 || fd_ >= 0) {
    return ErrorCode::kSystemError;
  }
  const std::filesystem::path fence_path(path);
  if (fence_path.has_parent_path()) {
    std::error_code error;
    std::filesystem::create_directories(fence_path.parent_path(), error);
    if (error) {
      return ErrorCode::kSystemError;
    }
  }

  // 2. 打开围栏文件并抢占非阻塞排他锁
  const int fd = ::open(path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0640);
  if (fd < 0) {
    return ErrorCode::kSystemError;
  }
  if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
    ::close(fd);
    return errno == EWOULDBLOCK ? ErrorCode::kResourceExhausted : ErrorCode::kSystemError;
  }

  // 3. 读取旧 epoch，写入 max(minimum, previous+1) 并 fsync
  std::uint64_t previous_epoch = 0;
  char buffer[64]{};
  const ssize_t size = ::pread(fd, buffer, sizeof(buffer) - 1, 0);
  if (size > 0) {
    const char* begin = buffer;
    const char* end = buffer + size;
    (void)std::from_chars(begin, end, previous_epoch);
  }
  const std::uint64_t epoch = std::max(minimum_epoch, previous_epoch + 1);
  const std::string serialized = std::to_string(epoch) + "\n";
  if (::ftruncate(fd, 0) != 0 ||
      ::pwrite(fd, serialized.data(), serialized.size(), 0) != static_cast<ssize_t>(serialized.size()) ||
      ::fsync(fd) != 0) {
    ::flock(fd, LOCK_UN);
    ::close(fd);
    return ErrorCode::kSystemError;
  }

  fd_ = fd;
  epoch_ = epoch;
  return ErrorCode::kSuccess;
}

void EngineFence::Release() {
  if (fd_ < 0) {
    return;
  }
  (void)::flock(fd_, LOCK_UN);
  (void)::close(fd_);
  fd_ = -1;
  epoch_ = 0;
}

std::uint64_t EngineFence::Epoch() const {
  return epoch_;
}

}  // namespace qtrade::engine
