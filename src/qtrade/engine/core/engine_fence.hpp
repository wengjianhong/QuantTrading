/// @file      engine_fence.hpp
/// @brief     单实例排他写锁与 engine_epoch 分配
/// @details   通过共享路径上的排他 flock 保证同一账户仅一个引擎进程可写；
///            持锁期间在文件中写入单调递增的 engine_epoch
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ENGINE_FENCE_HPP_
#define QTRADE_ENGINE_ENGINE_FENCE_HPP_

#include <qtrade/error_code/error_codes.hpp>

#include <cstdint>
#include <string>

namespace qtrade::engine {

/// @brief 基于共享文件锁的单账户引擎写锁
/// @details 持有排他 flock 期间同一路径仅允许一个引擎进程；文件内保存单调 epoch
class EngineFence {
 public:
  /// @brief 析构并释放写锁
  ~EngineFence();

  EngineFence() = default;
  EngineFence(const EngineFence&) = delete;
  EngineFence& operator=(const EngineFence&) = delete;

  /// @brief 获取排他写锁并分配新 epoch
  /// @param path 锁文件路径；多实例必须指向同一共享文件
  /// @param minimum_epoch 配置要求的最小 epoch（须 > 0）
  /// @return 成功返回 kSuccess；他实例已持锁返回 kResourceExhausted；
  ///         参数非法、目录/IO/fsync 失败返回 kSystemError
  ErrorCode Acquire(const std::string& path, std::uint64_t minimum_epoch);

  /// @brief 释放排他锁并关闭锁文件描述符
  void Release();

  /// @brief 查询本次分配的 engine_epoch
  /// @return 持锁时返回已写入的 epoch；未持锁时返回 0
  [[nodiscard]] std::uint64_t Epoch() const;

 private:
  /// 锁文件描述符；生命周期内保持打开
  int fd_ = -1;
  /// 本次分配的 epoch
  std::uint64_t epoch_ = 0;
};

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_ENGINE_FENCE_HPP_
