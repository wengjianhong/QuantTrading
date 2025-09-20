#pragma once

#include <string>
#include <cstdint>
#include <chrono>

namespace quant {
namespace shared {
namespace data_types {

// 标准化Tick数据结构
struct TickData {
    std::string instrument;          // 合约代码
    std::chrono::system_clock::time_point timestamp;  // 时间戳
    double last_price;               // 最新价
    int64_t volume;                  // 成交量
    double open_interest;            // 持仓量
    double bid_price[5];             // 买一到买五价
    int32_t bid_volume[5];           // 买一到买五量
    double ask_price[5];             // 卖一到卖五价
    int32_t ask_volume[5];           // 卖一到卖五量
    double open_price;               // 开盘价
    double high_price;               // 最高价
    double low_price;                // 最低价
    double pre_close_price;          // 昨收盘价
};

// 原始Tick数据（未标准化）
struct RawTickData {
    std::string data_source;         // 数据源名称
    std::string raw_data;            // 原始数据
};

} // namespace data_types
} // namespace shared
} // namespace quant
