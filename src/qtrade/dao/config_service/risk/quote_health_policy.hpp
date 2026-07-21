/// @file      quote_health_policy.hpp
/// @brief     quote_health_policy 表 DAO 声明（§3.2 行情健康处置策略表）
/// @details   由 config-service 持久化并经 ConfigSnapshot 下发；引擎只读本地快照。当前仅提供 Record 与 DDL；DML
/// 待后续实现
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_DAO_QUOTE_HEALTH_POLICY_HPP_
#define QTRADE_DAO_QUOTE_HEALTH_POLICY_HPP_

#include <qtrade/dao/ddl.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace qtrade::framework::dao {

/// @brief quote_health_policy 表行记录
/// @details 表主键说明：(tenant_id, engine_id, instrument_id)
///
struct QuoteHealthPolicyRecord {
  /// 租户 ID
  std::optional<std::string> tenant_id;
  /// 引擎实例 ID
  std::optional<std::string> engine_id;
  /// 品种 ID；空串表示引擎缺省策略
  std::optional<std::string> instrument_id;
  /// 策略配置版本
  std::optional<std::uint64_t> version;
  /// 行情延迟上限（毫秒）
  std::optional<std::int64_t> max_quote_latency_ms;
  /// 行情时间戳陈旧上限（毫秒）
  std::optional<std::int64_t> max_stale_age_ms;
  /// 是否允许序号缺口继续交易
  std::optional<bool> allow_sequence_gap;
  /// 行情源切换期间是否拒绝新开仓
  std::optional<bool> reject_on_failover;
  /// 不健康时是否拒绝受影响品种新开仓
  std::optional<bool> reject_new_open_on_unhealthy;
  /// 涨跌停状态是否禁止新开仓
  std::optional<bool> block_open_on_limit_up_down;
  /// 是否启用行情健康校验
  std::optional<bool> enabled;
};

/// @brief quote_health_policy 表 DDL
/// @details 实现 ITableDdl；增删改查尚未实现
class QuoteHealthPolicy final : public ITableDdl {
 public:
  QuoteHealthPolicy() = default;
  QuoteHealthPolicy(QuoteHealthPolicy&&) = delete;
  QuoteHealthPolicy(const QuoteHealthPolicy&) = delete;
  QuoteHealthPolicy& operator=(QuoteHealthPolicy&&) = delete;
  QuoteHealthPolicy& operator=(const QuoteHealthPolicy&) = delete;
  ~QuoteHealthPolicy() noexcept override = default;

  /// @brief 获取逻辑数据库名
  /// @return 固定为 "config"
  const std::string& DatabaseName() const override;

  /// @brief 获取逻辑表名
  /// @return 固定为 "quote_health_policy"
  const std::string& TableName() const override;

  /// @brief 获取建表 SQL 列表
  /// @return 含 CREATE TABLE 语句的列表
  const std::vector<std::string>& GetCreateTableSqls() const override;

  /// @brief 获取索引 SQL 列表
  /// @return 索引语句列表；无索引时为空
  const std::vector<std::string>& GetIndexSqls() const override;
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_DAO_QUOTE_HEALTH_POLICY_HPP_
