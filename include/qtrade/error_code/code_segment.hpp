/// @file      code_segment.hpp
/// @brief     错误码分段定义
/// @details   定义错误码的 AA 系统编号和 BBB 模块编号
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ERROR_CODE_CODE_SEGMENT_HPP_
#define QTRADE_ERROR_CODE_CODE_SEGMENT_HPP_
#include <cpputils/error_code/error_code_layout.hpp>

#include <cstdint>

namespace qtrade {
using cpputils::error_code::MakeModuleId;
using cpputils::error_code::MakeServiceId;

/// @brief 系统级编号（AAA=1~999）
enum class SystemNumber : uint64_t {
  /// 通用错误码
  kCommon = 0,
  /// qtrade系统
  kQTrade = 1,
  /// 服务级编号结束标记
  kEnd = 2,
};

/// @brief 服务编号
enum class ServiceNumber : uint64_t {
  /// ============================ 通用错误码 ============================
  /// 通用错误码
  kCommon = MakeServiceId(static_cast<uint64_t>(SystemNumber::kCommon), 0),

  /// ============================ qtrade系统 ============================
  /// 核心交易引擎服务
  kEngine = MakeServiceId(static_cast<uint64_t>(SystemNumber::kQTrade), 1),
  /// 账户服务
  kAccount = MakeServiceId(static_cast<uint64_t>(SystemNumber::kQTrade), 2),
  /// 账户风控服务
  kAccountRisk = MakeServiceId(static_cast<uint64_t>(SystemNumber::kQTrade), 3),
  /// 配置服务
  kConfig = MakeServiceId(static_cast<uint64_t>(SystemNumber::kQTrade), 4),
  /// 日志服务
  kLog = MakeServiceId(static_cast<uint64_t>(SystemNumber::kQTrade), 5),
  /// 监控服务
  kMonitor = MakeServiceId(static_cast<uint64_t>(SystemNumber::kQTrade), 6),
  /// 注册服务
  kRegistry = MakeServiceId(static_cast<uint64_t>(SystemNumber::kQTrade), 7),
  /// 审计服务
  kAudit = MakeServiceId(static_cast<uint64_t>(SystemNumber::kQTrade), 8),
  /// 回测服务
  kBacktest = MakeServiceId(static_cast<uint64_t>(SystemNumber::kQTrade), 9),
  /// 备份服务
  kBackup = MakeServiceId(static_cast<uint64_t>(SystemNumber::kQTrade), 10),
  /// 历史行情服务
  kHistoryMarket = MakeServiceId(static_cast<uint64_t>(SystemNumber::kQTrade), 11),
  /// 历史订单服务
  kHistoryOrder = MakeServiceId(static_cast<uint64_t>(SystemNumber::kQTrade), 12),
  /// 策略管理服务
  kStrategy = MakeServiceId(static_cast<uint64_t>(SystemNumber::kQTrade), 13),
  /// 服务级编号结束标记
  kEnd = MakeServiceId(static_cast<uint64_t>(SystemNumber::kQTrade), 14),
};

/// @brief 模块编号
/// @details 模块编号在各服务内独立复用；当前枚举先定义公共模块与qtrade系统模块。
enum class ModuleNumber : uint64_t {
  /// ============================ 通用错误码 ============================
  /// 通用错误码模块
  kCommon = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kCommon), 0),
  /// 系统模块
  kSystemError = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kCommon), 1),
  /// 网络错误码模块
  kNetworkError = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kCommon), 2),
  /// SQL 错误码模块
  kSqlError = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kCommon), 3),

  /// ============================ 核心交易引擎服务 ============================
  /// 行情适配器模块
  kQuoteAdapter = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kEngine), 0),
  /// 交易执行适配器模块
  kTraderAdapter = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kEngine), 1),
  /// 交易引擎通用错误码段
  kEngineCommon = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kEngine), 2),
  /// 账号管理模块
  kAccount = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kEngine), 3),
  /// 事件总线模块
  kEventBus = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kEngine), 4),
  /// 交易合规模块(cms)
  kCompliance = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kEngine), 5),
  /// 交易执行模块(ems)
  kExecution = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kEngine), 6),
  /// 订单管理模块(oms)
  kOrder = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kEngine), 7),
  /// 行情标准化模块（QuoteNormalizer）
  kQuoteNormalizer = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kEngine), 8),
  /// 持仓管理模块
  kPosition = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kEngine), 9),
  /// 风险管理模块
  kRisk = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kEngine), 10),
  /// 策略引擎模块
  kStrategyEngine = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kEngine), 11),
  /// 交易标准化模块（TraderNormalizer）
  kTraderNormalizer = MakeModuleId(static_cast<uint64_t>(ServiceNumber::kEngine), 12),
};

}  // namespace qtrade

#endif  // QTRADE_ERROR_CODE_CODE_SEGMENT_HPP_
