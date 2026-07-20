/// @file      dao_define.hpp
/// @brief     DAO 层共享逻辑数据库名称
#ifndef QTRADE_DAO_DAO_DEFINE_HPP_
#define QTRADE_DAO_DAO_DEFINE_HPP_

#include <string>

namespace qtrade::framework::dao {

/// @brief 交易账户数据 数据库名称
inline const std::string kAccountDatabaseName = "account";
/// @brief 引擎配置与通用风控策略所在的逻辑数据库名称
inline const std::string kConfigDatabaseName = "config";
/// @brief 账户硬风控数据所在的逻辑数据库名称
inline const std::string kAccountRiskDatabaseName = "account_risk";

}  // namespace qtrade::framework::dao

#endif  // QTRADE_DAO_DAO_DEFINE_HPP_
