/// @file      order_journal.cpp
/// @brief     订单主日志实现
/// @details   JSON Lines 追加写、回放与序列化；作为 OMS 崩溃恢复事实源
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/oms/order_journal.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <string_view>
#include <unistd.h>

namespace qtrade::engine::oms {
namespace {

using json = nlohmann::json;
using qtrade_sdk::trader::Order;
using qtrade_sdk::trader::Trade;

/// @brief 将枚举转换为 JSON 整数
/// @tparam Enum 枚举类型
/// @param value 枚举值
/// @return 枚举底层整数值
template <typename Enum>
[[nodiscard]] int EnumValue(Enum value) {
  return static_cast<int>(value);
}

/// @brief 将订单转换为 JSON
/// @param order 订单快照
/// @return JSON 对象
[[nodiscard]] json OrderToJson(const Order& order) {
  return {
    {"order_id", order.order_id},
    {"order_emt_id", order.order_emt_id},
    {"client_order_id", order.client_order_id},
    {"exchange_order_id", order.exchange_order_id},
    {"instrument", order.instrument},
    {"market", EnumValue(order.market)},
    {"price", order.price},
    {"volume", order.volume},
    {"traded_volume", order.traded_volume},
    {"left_volume", order.left_volume},
    {"trade_amount", order.trade_amount},
    {"price_type", EnumValue(order.price_type)},
    {"side", EnumValue(order.side)},
    {"position_effect", EnumValue(order.position_effect)},
    {"business_type", EnumValue(order.business_type)},
    {"status", EnumValue(order.status)},
    {"submit_status", EnumValue(order.submit_status)},
    {"insert_time", order.insert_time},
    {"update_time", order.update_time},
    {"cancel_time", order.cancel_time},
    {"error_message", order.error_message},
  };
}

/// @brief 从 JSON 解析订单
/// @param value JSON 对象
/// @return 订单快照
[[nodiscard]] Order OrderFromJson(const json& value) {
  Order order;
  order.order_id = value.value("order_id", "");
  order.order_emt_id = value.value("order_emt_id", std::uint64_t{0});
  order.client_order_id = value.value("client_order_id", std::uint32_t{0});
  order.exchange_order_id = value.value("exchange_order_id", "");
  order.instrument = value.value("instrument", "");
  order.market = static_cast<qtrade_sdk::trader::MarketType>(value.value("market", 99));
  order.price = value.value("price", 0.0);
  order.volume = value.value("volume", std::int64_t{0});
  order.traded_volume = value.value("traded_volume", std::int64_t{0});
  order.left_volume = value.value("left_volume", std::int64_t{0});
  order.trade_amount = value.value("trade_amount", 0.0);
  order.price_type = static_cast<qtrade_sdk::trader::PriceType>(value.value("price_type", 255));
  order.side = static_cast<qtrade_sdk::trader::SideType>(value.value("side", 50));
  order.position_effect =
    static_cast<qtrade_sdk::trader::PositionEffectType>(value.value("position_effect", 12));
  order.business_type = static_cast<qtrade_sdk::trader::BusinessType>(value.value("business_type", 255));
  order.status = static_cast<qtrade_sdk::trader::OrderStatusType>(value.value("status", 255));
  order.submit_status =
    static_cast<qtrade_sdk::trader::OrderSubmitStatusType>(value.value("submit_status", 0));
  order.insert_time = value.value("insert_time", std::int64_t{0});
  order.update_time = value.value("update_time", std::int64_t{0});
  order.cancel_time = value.value("cancel_time", std::int64_t{0});
  order.error_message = value.value("error_message", "");
  return order;
}

/// @brief 将成交转换为 JSON
/// @param trade 成交回报
/// @return JSON 对象
[[nodiscard]] json TradeToJson(const Trade& trade) {
  return {
    {"trade_id", trade.trade_id},
    {"order_id", trade.order_id},
    {"order_emt_id", trade.order_emt_id},
    {"client_order_id", trade.client_order_id},
    {"exchange_order_id", trade.exchange_order_id},
    {"instrument", trade.instrument},
    {"market", EnumValue(trade.market)},
    {"price", trade.price},
    {"volume", trade.volume},
    {"trade_amount", trade.trade_amount},
    {"trade_time", trade.trade_time},
    {"side", EnumValue(trade.side)},
    {"position_effect", EnumValue(trade.position_effect)},
    {"trade_type", EnumValue(trade.trade_type)},
    {"business_type", EnumValue(trade.business_type)},
    {"report_index", trade.report_index},
  };
}

/// @brief 从 JSON 解析成交
/// @param value JSON 对象
/// @return 成交回报
[[nodiscard]] Trade TradeFromJson(const json& value) {
  Trade trade;
  trade.trade_id = value.value("trade_id", "");
  trade.order_id = value.value("order_id", "");
  trade.order_emt_id = value.value("order_emt_id", std::uint64_t{0});
  trade.client_order_id = value.value("client_order_id", std::uint32_t{0});
  trade.exchange_order_id = value.value("exchange_order_id", "");
  trade.instrument = value.value("instrument", "");
  trade.market = static_cast<qtrade_sdk::trader::MarketType>(value.value("market", 99));
  trade.price = value.value("price", 0.0);
  trade.volume = value.value("volume", std::int64_t{0});
  trade.trade_amount = value.value("trade_amount", 0.0);
  trade.trade_time = value.value("trade_time", std::int64_t{0});
  trade.side = static_cast<qtrade_sdk::trader::SideType>(value.value("side", 50));
  trade.position_effect =
    static_cast<qtrade_sdk::trader::PositionEffectType>(value.value("position_effect", 12));
  trade.trade_type = static_cast<qtrade_sdk::trader::TradeType>(value.value("trade_type", 255));
  trade.business_type = static_cast<qtrade_sdk::trader::BusinessType>(value.value("business_type", 255));
  trade.report_index = value.value("report_index", std::uint64_t{0});
  return trade;
}

/// @brief 将日志记录序列化为单行 JSON
/// @param record 日志记录
/// @return 不含末尾换行的 JSON 字符串
[[nodiscard]] std::string SerializeRecord(const OrderJournalRecord& record) {
  json value = {
    {"sequence", record.sequence},
    {"timestamp_ns", record.timestamp_ns},
    {"engine_epoch", record.engine_epoch},
    {"event_type", EnumValue(record.event_type)},
    {"lifecycle_state", EnumValue(record.lifecycle_state)},
    {"order", OrderToJson(record.order)},
    {"message", record.message},
  };
  if (record.trade.has_value()) {
    value["trade"] = TradeToJson(*record.trade);
  }
  return value.dump();
}

/// @brief 从单行 JSON 解析日志记录
/// @param line JSON Lines 中的一行
/// @return 解析成功返回记录
[[nodiscard]] std::optional<OrderJournalRecord> ParseRecord(std::string_view line) {
  try {
    const json value = json::parse(line);
    if (!value.is_object() || !value.contains("order")) {
      return std::nullopt;
    }
    OrderJournalRecord record;
    record.sequence = value.value("sequence", std::uint64_t{0});
    record.timestamp_ns = value.value("timestamp_ns", std::uint64_t{0});
    record.engine_epoch = value.value("engine_epoch", std::uint64_t{0});
    record.event_type = static_cast<OrderJournalEventType>(value.value("event_type", 0));
    record.lifecycle_state = static_cast<OrderLifecycleState>(value.value("lifecycle_state", 0));
    record.order = OrderFromJson(value.at("order"));
    record.message = value.value("message", "");
    if (value.contains("trade") && value.at("trade").is_object()) {
      record.trade = TradeFromJson(value.at("trade"));
    }
    return record;
  } catch (const std::exception& error) {
    spdlog::error("[OrderJournal] ignored invalid record: {}", error.what());
    return std::nullopt;
  }
}

/// @brief 从文件读取全部有效日志记录
/// @param path 日志文件路径
/// @return 按文件顺序读取的有效记录
[[nodiscard]] std::vector<OrderJournalRecord> ReadRecords(const std::string& path) {
  std::vector<OrderJournalRecord> records;
  std::ifstream input(path);
  if (!input.is_open()) {
    return records;
  }
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    if (auto record = ParseRecord(line); record.has_value()) {
      records.push_back(std::move(*record));
    }
  }
  std::sort(records.begin(), records.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.sequence < rhs.sequence;
  });
  return records;
}

}  // namespace

OrderJournal::~OrderJournal() {
  Close();
}

ErrorCode OrderJournal::Open(const std::string& path, bool sync_on_append) {
  if (path.empty()) {
    return ErrorCode::kSystemError;
  }

  std::lock_guard lock(mutex_);
  if (fd_ >= 0) {
    return ErrorCode::kSystemError;
  }

  // 1. 确保父目录存在
  const std::filesystem::path journal_path(path);
  if (journal_path.has_parent_path()) {
    std::error_code error;
    std::filesystem::create_directories(journal_path.parent_path(), error);
    if (error) {
      spdlog::error("[OrderJournal] create directory failed: {}", error.message());
      return ErrorCode::kSystemError;
    }
  }

  // 2. 回放已有记录以恢复下一序号，再以追加模式打开
  path_ = path;
  sync_on_append_ = sync_on_append;
  const auto records = ReadRecords(path_);
  if (!records.empty()) {
    next_sequence_ = records.back().sequence + 1;
  }

  fd_ = ::open(path_.c_str(), O_CREAT | O_APPEND | O_WRONLY | O_CLOEXEC, 0640);
  if (fd_ < 0) {
    spdlog::error("[OrderJournal] open failed: path={}, errno={}", path_, errno);
    path_.clear();
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

void OrderJournal::Close() {
  std::lock_guard lock(mutex_);
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

ErrorCode OrderJournal::Append(OrderJournalRecord record) {
  std::lock_guard lock(mutex_);
  if (fd_ < 0) {
    return ErrorCode::kNotInitialized;
  }

  // 1. 覆盖序号与时间戳后序列化
  record.sequence = next_sequence_;
  record.timestamp_ns = static_cast<std::uint64_t>(
    std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count());
  std::string line = SerializeRecord(record);
  line.push_back('\n');

  // 2. 完整写出一行，必要时 fsync
  std::size_t offset = 0;
  while (offset < line.size()) {
    const ssize_t written = ::write(fd_, line.data() + offset, line.size() - offset);
    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      spdlog::error("[OrderJournal] write failed: path={}, errno={}", path_, errno);
      return ErrorCode::kSystemError;
    }
    offset += static_cast<std::size_t>(written);
  }

  if (sync_on_append_ && ::fsync(fd_) != 0) {
    spdlog::error("[OrderJournal] fsync failed: path={}, errno={}", path_, errno);
    return ErrorCode::kSystemError;
  }
  ++next_sequence_;
  return ErrorCode::kSuccess;
}

std::vector<OrderJournalRecord> OrderJournal::Replay() const {
  std::lock_guard lock(mutex_);
  return path_.empty() ? std::vector<OrderJournalRecord>{} : ReadRecords(path_);
}

bool OrderJournal::IsOpen() const {
  std::lock_guard lock(mutex_);
  return fd_ >= 0;
}

}  // namespace qtrade::engine::oms
