/// @file      systemd_notify.hpp
/// @brief     systemd 服务状态通知（参考 ugos_serv common/system/systemd_notify）
/// @details   未设置 NOTIFY_SOCKET 时为空操作，便于非 systemd 环境本地运行
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_SYSTEM_SYSTEMD_NOTIFY_HPP_
#define QTRADE_COMMON_SYSTEM_SYSTEMD_NOTIFY_HPP_

#include <string>

namespace qtrade::common::system {

/// @brief 通知 systemd 服务状态（自动加 STATUS= 前缀）
/// @param status 状态消息（会自动添加 STATUS= 前缀）
/// @return 成功返回 true，失败返回 false
bool NotifyStatus(const std::string& status);

/// @brief 通知 systemd 服务就绪
/// @param status 附加状态描述（可选）
/// @return 成功返回 true，失败返回 false
bool NotifyReady(const std::string& status = "");

/// @brief 通知 systemd 服务正在停止
/// @param status 附加状态描述（可选）
/// @return 成功返回 true，失败返回 false
bool NotifyStopping(const std::string& status = "");

/// @brief 通知 systemd 服务错误
/// @param errno_value 错误码（0 表示不设置 ERRNO=）
/// @param error_msg 错误消息（写入 STATUS=）
/// @return 成功返回 true，失败返回 false
bool NotifyError(int errno_value, const std::string& error_msg);

}  // namespace qtrade::common::system

#endif  // QTRADE_COMMON_SYSTEM_SYSTEMD_NOTIFY_HPP_
