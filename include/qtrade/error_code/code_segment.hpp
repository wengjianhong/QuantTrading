/// @file      code_segment.hpp
/// @brief     错误码分段定义
/// @details   定义错误码的 AA 系统编号和 BBB 模块编号
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ERROR_CODE_CODE_SEGMENT_HPP_
#define QTRADE_ERROR_CODE_CODE_SEGMENT_HPP_
#include <cstdint>

namespace qtrade {

/// AA 级：系统编号（从 1 ~ 20）
enum class CodeSystem : int32_t {
  kQTradeSystem = 10,  /// qtrade 系统错误码段
  kEnd = 21,           /// 系统编号结束【int32_t max value: 2'147'483'647】
};

/// BBB 级模块编号
enum class ModuleNumber : int32_t {
  /// 系统层（BBB=000~099）
  kQTradeSystem = 0,  /// 通用错误
  kSystemError = 1,   /// 底层错误码
  kNetworkError = 2,  /// 网络错误码段
  kSqlError = 3,      /// SQL 错误码段

  /// 适配器层（BBB=100~199）
  kMarketAdapter = 100,     /// 行情适配器模块
  kExecutionAdapter = 101,  /// 交易执行适配器模块

  /// 交易引擎层（BBB=200~299）
  kEngineCommon = 200,      /// 交易引擎通用错误码段
  kAccount = 201,           /// 账号管理模块
  kEventBus = 202,          /// 事件总线模块
  kCompliance = 203,        /// 交易合规模块(cms)
  kExecution = 204,         /// 交易执行模块(ems)
  kOrder = 205,             /// 订单管理模块(oms)
  kQuoteNormalizer = 206,   /// 行情标准化模块（QuoteNormalizer）
  kPosition = 207,          /// 持仓管理模块
  kRisk = 208,              /// 风险管理模块
  kStrategyEngine = 209,    /// 策略引擎模块
  kTraderNormalizer = 210,  /// 交易标准化模块（TraderNormalizer）

  /// 支撑服务层（BBB=300-399）
  kServiceCommon = 300,  /// 支撑服务通用错误码段
  kAudit = 301,          /// 审计服务
  kBacktest = 302,       /// 回测服务
  kBackup = 303,         /// 备份服务
  kConfig = 304,         /// 配置服务
  kHistoryMarket = 305,  /// 历史行情服务
  kHistoryOrder = 306,   /// 历史订单服务
  kLog = 307,            /// 日志服务
  kMonitor = 308,        /// 监控服务
  kRegistry = 309,       /// 注册服务
  kStrategy = 310,       /// 策略管理服务
};

/// 构造 AABBBCCC 格式错误码（AA=10, BBB=module_number, CCC=detail）
inline constexpr int32_t MakeErrorCode(ModuleNumber module_number, int32_t code) {
  return static_cast<int32_t>(CodeSystem::kQTradeSystem) * 1000000 + static_cast<int32_t>(module_number) * 1000 + code;
}

/// CCC 级：具体错误码从 0 起 到 999 止
inline constexpr int32_t kRangeStart = 0;
inline constexpr int32_t kRangeEnd = 999;

/// 各模块错误码划段（每模块预留 999 个 CCC）
struct ModuleErrorCodeRange {
  /// 系统层（BBB=000~099）
  /// 通用错误码段（错误码范围=10000000~10000999）
  static constexpr int32_t kQTradeSystemStart = MakeErrorCode(ModuleNumber::kQTradeSystem, kRangeStart);
  static constexpr int32_t kQTradeSystemEnd = MakeErrorCode(ModuleNumber::kQTradeSystem, kRangeEnd);

  /// 底层错误码（错误码范围=10001000~10001999）
  static constexpr int32_t kSystemErrorStart = MakeErrorCode(ModuleNumber::kSystemError, kRangeStart);
  static constexpr int32_t kSystemErrorEnd = MakeErrorCode(ModuleNumber::kSystemError, kRangeEnd);

  /// 网络错误码段（错误码范围=10002000~10002999）
  static constexpr int32_t kNetworkErrorStart = MakeErrorCode(ModuleNumber::kNetworkError, kRangeStart);
  static constexpr int32_t kNetworkErrorEnd = MakeErrorCode(ModuleNumber::kNetworkError, kRangeEnd);

  /// SQL 错误码段（错误码范围=10003000~10003999）
  static constexpr int32_t kSqlErrorStart = MakeErrorCode(ModuleNumber::kSqlError, kRangeStart);
  static constexpr int32_t kSqlErrorEnd = MakeErrorCode(ModuleNumber::kSqlError, kRangeEnd);

  /// 适配器层（BBB=100~199）
  /// 行情适配器模块（错误码范围=10100000~10100999）
  static constexpr int32_t kMarketAdapterStart = MakeErrorCode(ModuleNumber::kMarketAdapter, kRangeStart);
  static constexpr int32_t kMarketAdapterEnd = MakeErrorCode(ModuleNumber::kMarketAdapter, kRangeEnd);

  /// 交易执行适配器模块（错误码范围=10101000~10101999）
  static constexpr int32_t kExecutionAdapterStart = MakeErrorCode(ModuleNumber::kExecutionAdapter, kRangeStart);
  static constexpr int32_t kExecutionAdapterEnd = MakeErrorCode(ModuleNumber::kExecutionAdapter, kRangeEnd);

  /// 交易引擎层（BBB=200~299）
  /// 交易引擎通用错误码段（错误码范围=10200000~10299999）
  static constexpr int32_t kEngineCommonStart = MakeErrorCode(ModuleNumber::kEngineCommon, kRangeStart);
  static constexpr int32_t kEngineCommonEnd = MakeErrorCode(ModuleNumber::kEngineCommon, kRangeEnd);

  /// 账号管理模块（错误码范围=10201000~10201999）
  static constexpr int32_t kAccountStart = MakeErrorCode(ModuleNumber::kAccount, kRangeStart);
  static constexpr int32_t kAccountEnd = MakeErrorCode(ModuleNumber::kAccount, kRangeEnd);

  /// 事件总线模块（错误码范围=10202000~10202999）
  static constexpr int32_t kEventBusStart = MakeErrorCode(ModuleNumber::kEventBus, kRangeStart);
  static constexpr int32_t kEventBusEnd = MakeErrorCode(ModuleNumber::kEventBus, kRangeEnd);

  /// 交易合规模块(cms)（错误码范围=10203000~10203999）
  static constexpr int32_t kComplianceStart = MakeErrorCode(ModuleNumber::kCompliance, kRangeStart);
  static constexpr int32_t kComplianceEnd = MakeErrorCode(ModuleNumber::kCompliance, kRangeEnd);

  /// 交易执行模块(ems)（错误码范围=10204000~10204999）
  static constexpr int32_t kExecutionStart = MakeErrorCode(ModuleNumber::kExecution, kRangeStart);
  static constexpr int32_t kExecutionEnd = MakeErrorCode(ModuleNumber::kExecution, kRangeEnd);

  /// 订单管理模块(oms)（错误码范围=10205000~10205999）
  static constexpr int32_t kOrderStart = MakeErrorCode(ModuleNumber::kOrder, kRangeStart);
  static constexpr int32_t kOrderEnd = MakeErrorCode(ModuleNumber::kOrder, kRangeEnd);

  /// 行情标准化模块 QuoteNormalizer（错误码范围=10206000~10206999）
  static constexpr int32_t kQuoteNormalizerStart = MakeErrorCode(ModuleNumber::kQuoteNormalizer, kRangeStart);
  static constexpr int32_t kQuoteNormalizerEnd = MakeErrorCode(ModuleNumber::kQuoteNormalizer, kRangeEnd);

  /// 持仓管理模块（错误码范围=10207000~10207999）
  static constexpr int32_t kPositionStart = MakeErrorCode(ModuleNumber::kPosition, kRangeStart);
  static constexpr int32_t kPositionEnd = MakeErrorCode(ModuleNumber::kPosition, kRangeEnd);

  /// 风险管理模块（错误码范围=10208000~10208999）
  static constexpr int32_t kRiskStart = MakeErrorCode(ModuleNumber::kRisk, kRangeStart);
  static constexpr int32_t kRiskEnd = MakeErrorCode(ModuleNumber::kRisk, kRangeEnd);

  /// 策略引擎模块（错误码范围=10209000~10209999）
  static constexpr int32_t kStrategyEngineStart = MakeErrorCode(ModuleNumber::kStrategyEngine, kRangeStart);
  static constexpr int32_t kStrategyEngineEnd = MakeErrorCode(ModuleNumber::kStrategyEngine, kRangeEnd);

  /// 交易标准化模块 TraderNormalizer（错误码范围=10210000~10210999）
  static constexpr int32_t kTraderNormalizerStart = MakeErrorCode(ModuleNumber::kTraderNormalizer, kRangeStart);
  static constexpr int32_t kTraderNormalizerEnd = MakeErrorCode(ModuleNumber::kTraderNormalizer, kRangeEnd);

  // 支撑服务层（BBB=300~399）
  /// 支撑服务通用错误码段（错误码范围=10300000~10300999）
  static constexpr int32_t kServiceCommonStart = MakeErrorCode(ModuleNumber::kServiceCommon, kRangeStart);
  static constexpr int32_t kServiceCommonEnd = MakeErrorCode(ModuleNumber::kServiceCommon, kRangeEnd);

  /// 审计服务（错误码范围=10301000~10301999）
  static constexpr int32_t kAuditStart = MakeErrorCode(ModuleNumber::kAudit, kRangeStart);
  static constexpr int32_t kAuditEnd = MakeErrorCode(ModuleNumber::kAudit, kRangeEnd);

  /// 回测服务（错误码范围=10302000~10302999）
  static constexpr int32_t kBacktestStart = MakeErrorCode(ModuleNumber::kBacktest, kRangeStart);
  static constexpr int32_t kBacktestEnd = MakeErrorCode(ModuleNumber::kBacktest, kRangeEnd);

  /// 备份服务（错误码范围=10303000~10303999）
  static constexpr int32_t kBackupStart = MakeErrorCode(ModuleNumber::kBackup, kRangeStart);
  static constexpr int32_t kBackupEnd = MakeErrorCode(ModuleNumber::kBackup, kRangeEnd);

  /// 配置服务（错误码范围=10304000~10304999）
  static constexpr int32_t kConfigStart = MakeErrorCode(ModuleNumber::kConfig, kRangeStart);
  static constexpr int32_t kConfigEnd = MakeErrorCode(ModuleNumber::kConfig, kRangeEnd);

  /// 历史行情服务（错误码范围=10305000~10305999）
  static constexpr int32_t kHistoryMarketStart = MakeErrorCode(ModuleNumber::kHistoryMarket, kRangeStart);
  static constexpr int32_t kHistoryMarketEnd = MakeErrorCode(ModuleNumber::kHistoryMarket, kRangeEnd);

  /// 历史订单服务（错误码范围=10306000~10306999）
  static constexpr int32_t kHistoryOrderStart = MakeErrorCode(ModuleNumber::kHistoryOrder, kRangeStart);
  static constexpr int32_t kHistoryOrderEnd = MakeErrorCode(ModuleNumber::kHistoryOrder, kRangeEnd);

  /// 日志服务（错误码范围=10307000~10307999）
  static constexpr int32_t kLogStart = MakeErrorCode(ModuleNumber::kLog, kRangeStart);
  static constexpr int32_t kLogEnd = MakeErrorCode(ModuleNumber::kLog, kRangeEnd);

  /// 监控服务（错误码范围=10308000~10308999）
  static constexpr int32_t kMonitorStart = MakeErrorCode(ModuleNumber::kMonitor, kRangeStart);
  static constexpr int32_t kMonitorEnd = MakeErrorCode(ModuleNumber::kMonitor, kRangeEnd);

  /// 注册服务（错误码范围=10309000~10309999）
  static constexpr int32_t kRegistryStart = MakeErrorCode(ModuleNumber::kRegistry, kRangeStart);
  static constexpr int32_t kRegistryEnd = MakeErrorCode(ModuleNumber::kRegistry, kRangeEnd);

  /// 策略管理服务（错误码范围=10310000~10310999）
  static constexpr int32_t kStrategyStart = MakeErrorCode(ModuleNumber::kStrategy, kRangeStart);
  static constexpr int32_t kStrategyEnd = MakeErrorCode(ModuleNumber::kStrategy, kRangeEnd);
};

}  // namespace qtrade

#endif  // QTRADE_ERROR_CODE_CODE_SEGMENT_HPP_
