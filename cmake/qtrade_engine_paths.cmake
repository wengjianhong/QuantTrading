# ---------------------------------------------------------------------------
# Project paths (centralized; keep CMake out of src/)
#
# Physical layout: src/qtrade_engine/（含 adapter/mock 供单测）
# Public headers: include/qtrade/...（含 sdk/）
# ---------------------------------------------------------------------------

set(QTRADE_ENGINE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(QTRADE_ENGINE_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(QTRADE_ENGINE_SRC_QTRADE_DIR ${QTRADE_ENGINE_SRC_DIR}/qtrade_engine)

# Map src/qtrade_engine -> build/include_overlay/qtrade so #include <qtrade/...>
# keeps working while the physical directory matches the project name.
set(QTRADE_ENGINE_INCLUDE_OVERLAY_DIR ${CMAKE_BINARY_DIR}/include_overlay)
file(MAKE_DIRECTORY ${QTRADE_ENGINE_INCLUDE_OVERLAY_DIR})
set(_qtrade_engine_overlay_link "${QTRADE_ENGINE_INCLUDE_OVERLAY_DIR}/qtrade")
if(EXISTS "${_qtrade_engine_overlay_link}")
  file(REMOVE "${_qtrade_engine_overlay_link}")
endif()
file(CREATE_LINK
  "${QTRADE_ENGINE_SRC_QTRADE_DIR}"
  "${_qtrade_engine_overlay_link}"
  SYMBOLIC
)
unset(_qtrade_engine_overlay_link)
