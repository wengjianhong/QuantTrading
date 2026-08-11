/// @file      account_bridge.hpp
/// @brief     账户桥接接口与凭证材料结构
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_BRIDGE_ACCOUNT_BRIDGE_HPP_
#define QTRADE_BRIDGE_ACCOUNT_BRIDGE_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/structs/result.hpp>

#include <string>

namespace qtrade::account {

/// @brief 仅供已授权引擎建立柜台连接的短生命周期凭证材料
/// @details 不得用于账户查询/列表；调用方不得记录或转发 password。
struct CredentialMaterial {
  /// 交易账户号（全局唯一）
  std::string account_id;
  /// 交易柜台标识
  std::string broker_id;
  /// 柜台网关连接参数（不含登录密码）
  std::string connection_string;
  /// 交易登录密码
  std::string password;
};

/// @brief 账户桥接器
/// @details 注入引擎前须已可用；连接等生命周期由实现方 / 持有方管理，本接口不包含 Start/Stop。
class IAccountBridge {
 public:
  virtual ~IAccountBridge() = default;

  /// @brief 读取建立柜台连接所需的凭证材料
  /// @param account_id 交易账户号（全局唯一）
  /// @param engine_id 引擎实例标识
  /// @return Result<CredentialMaterial> 凭证材料
  virtual Result<CredentialMaterial> GetCredential(const std::string& account_id,
                                                   const std::string& engine_id) const = 0;
};

}  // namespace qtrade::account

#endif  // QTRADE_BRIDGE_ACCOUNT_BRIDGE_HPP_
