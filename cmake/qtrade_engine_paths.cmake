# ---------------------------------------------------------------------------
# Project paths (centralized; keep CMake out of src/)
#
# Physical layout: src/qtrade_engine/{common, account, oms, ...}（扁平，无嵌套 engine/）
# Public headers: include/qtrade/...（含 sdk/）
# 适配器实现在 qtrade_client；本仓仅 test/stubs/ 供单测（不进 lib）
#
# Build overlay keeps #include "qtrade/engine/..." and "qtrade/common/..." stable:
#   include_overlay/qtrade/common → src/qtrade_engine/common
#   include_overlay/qtrade/engine → src/qtrade_engine（整树；与 include/qtrade/engine/engine.hpp 并存）
# ---------------------------------------------------------------------------

set(QTRADE_ENGINE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(QTRADE_ENGINE_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(QTRADE_ENGINE_SRC_QTRADE_DIR ${QTRADE_ENGINE_SRC_DIR}/qtrade_engine)

set(QTRADE_ENGINE_INCLUDE_OVERLAY_DIR ${CMAKE_BINARY_DIR}/include_overlay)
file(MAKE_DIRECTORY ${QTRADE_ENGINE_INCLUDE_OVERLAY_DIR})

# Drop legacy whole-tree symlink (older: overlay/qtrade → src/qtrade_engine).
set(_qtrade_overlay_root "${QTRADE_ENGINE_INCLUDE_OVERLAY_DIR}/qtrade")
if(EXISTS "${_qtrade_overlay_root}" AND NOT IS_DIRECTORY "${_qtrade_overlay_root}")
  file(REMOVE "${_qtrade_overlay_root}")
elseif(IS_SYMLINK "${_qtrade_overlay_root}")
  file(REMOVE "${_qtrade_overlay_root}")
endif()
file(MAKE_DIRECTORY ${_qtrade_overlay_root})

set(_qtrade_overlay_common "${_qtrade_overlay_root}/common")
set(_qtrade_overlay_engine "${_qtrade_overlay_root}/engine")
foreach(_link IN ITEMS ${_qtrade_overlay_common} ${_qtrade_overlay_engine})
  if(EXISTS "${_link}" OR IS_SYMLINK "${_link}")
    file(REMOVE "${_link}")
  endif()
endforeach()

file(CREATE_LINK
  "${QTRADE_ENGINE_SRC_QTRADE_DIR}/common"
  "${_qtrade_overlay_common}"
  SYMBOLIC
)
file(CREATE_LINK
  "${QTRADE_ENGINE_SRC_QTRADE_DIR}"
  "${_qtrade_overlay_engine}"
  SYMBOLIC
)
unset(_qtrade_overlay_root)
unset(_qtrade_overlay_common)
unset(_qtrade_overlay_engine)
