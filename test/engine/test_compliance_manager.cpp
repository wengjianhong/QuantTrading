#include "qtrade/engine/compliance/compliance_manager.hpp"

#include <gtest/gtest.h>

TEST(ComplianceManager, PlaceholderAllowsEveryOrder) {
  qtrade::engine::compliance::ComplianceManager manager;
  qtrade::sdk::trader::OrderRequest request;
  request.instrument = "IF2506";
  EXPECT_EQ(manager.CheckOrder(request), qtrade::ErrorCode::kSuccess);
}

TEST(ComplianceManager, PlaceholderDoesNotApplyOrderPolicy) {
  qtrade::engine::compliance::ComplianceManager manager;

  qtrade::sdk::trader::OrderRequest request;
  request.instrument = "";
  EXPECT_EQ(manager.CheckOrder(request), qtrade::ErrorCode::kSuccess);
}
