# ---------------------------------------------------------------------------
# Entry point for find_package(qtrade_engine CONFIG).
# Defines imported target: qtrade_engine::qtrade_engine
# ---------------------------------------------------------------------------

include(CMakeFindDependencyMacro)

find_dependency(Threads)
find_dependency(spdlog 1.16.0)
find_dependency(nlohmann_json 3.11.0)
find_dependency(cpputils 0.1.0)

include("${CMAKE_CURRENT_LIST_DIR}/qtrade_engineTargets.cmake")
