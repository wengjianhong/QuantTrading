# ---------------------------------------------------------------------------
# Strategy plugin: example_strategy → libexample_strategy.so
# ---------------------------------------------------------------------------

set(_QTRADE_EXAMPLE_STRATEGY_DIR ${QTRADE_DEMO_STRATEGY_DIR}/example_strategy)

add_library(example_strategy SHARED
  ${_QTRADE_EXAMPLE_STRATEGY_DIR}/example_strategy.cpp
  ${_QTRADE_EXAMPLE_STRATEGY_DIR}/example_strategy_plugin.cpp
)
target_include_directories(example_strategy PRIVATE
  ${_QTRADE_EXAMPLE_STRATEGY_DIR}
  ${QTRADE_INCLUDE_DIR}
)
target_link_libraries(example_strategy PRIVATE
  qtrade_common
)
target_compile_options(example_strategy PRIVATE -fvisibility=hidden)
set_target_properties(example_strategy PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${QTRADE_STRATEGY_PLUGIN_OUTPUT_DIR}
  OUTPUT_NAME example_strategy
  BUILD_RPATH "\$ORIGIN"
)

install(TARGETS example_strategy
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/strategies
)

unset(_QTRADE_EXAMPLE_STRATEGY_DIR)
