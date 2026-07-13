# ---------------------------------------------------------------------------
# Project paths (centralized; keep CMake out of src/)
# Naming: cmake/*.cmake uses qtrade_ prefix + snake_case
# ---------------------------------------------------------------------------

set(QTRADE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(QTRADE_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(QTRADE_SRC_QTRADE_DIR ${QTRADE_SRC_DIR}/qtrade)
set(QTRADE_SRC_QTRADE_SDK_DIR ${QTRADE_SRC_DIR}/qtrade_sdk)
set(QTRADE_SRC_QTRADE_FRAMEWORK_DIR ${QTRADE_SRC_QTRADE_DIR}/framework)
set(QTRADE_DEMO_DIR ${CMAKE_SOURCE_DIR}/demo)
set(QTRADE_APPS_DIR ${QTRADE_SRC_QTRADE_DIR}/apps)
