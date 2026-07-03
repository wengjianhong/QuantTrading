cmake_minimum_required(VERSION 3.22)
project(qtrade VERSION 0.1.0 LANGUAGES CXX)

# -----------------------------------------------------------------------------
# Toolchain defaults
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Installed binaries embed RUNPATH so /usr/local/lib deps resolve without ldconfig.
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

add_compile_options(-fPIC -Wall -Wextra -Wpedantic -O2)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
find_package(Threads REQUIRED)
find_package(spdlog 1.16.0 REQUIRED)
find_package(nlohmann_json 3.11.0 REQUIRED)
find_package(cpputils 0.1.0 REQUIRED CONFIG)

# -----------------------------------------------------------------------------
# Libraries and applications
# -----------------------------------------------------------------------------
include(${CMAKE_SOURCE_DIR}/cmake/qtrade_paths.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/lib/qtrade_proto.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/lib/qtrade_common.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/lib/qtrade_core.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/app/qtrade_engine.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/app/qtrade_services.cmake)

# -----------------------------------------------------------------------------
# Install (downstream: find_package(qtrade CONFIG))
# -----------------------------------------------------------------------------
include(${CMAKE_SOURCE_DIR}/cmake/qtrade_install.cmake)

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------
enable_testing()
add_subdirectory(test)
