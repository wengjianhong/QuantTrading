/// @file      strategy_smoke_test.cpp
/// @brief     策略模块冒烟测试
/// @details   验证策略接口头文件可编译链接
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include <gtest/gtest.h>

#include <qtrade/strategy/strategy.hpp>

TEST(StrategySmoke, InterfaceVisible) { SUCCEED(); }
