# ---------------------------------------------------------------------------
# Strategy plugins aggregator: one cmake per strategy under cmake/strategy/
# ---------------------------------------------------------------------------

set(QTRADE_STRATEGY_PLUGIN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/lib/strategies)

include(${CMAKE_CURRENT_LIST_DIR}/example_strategy.cmake)

# 新增策略时在此追加：
# include(${CMAKE_CURRENT_LIST_DIR}/your_strategy.cmake)
