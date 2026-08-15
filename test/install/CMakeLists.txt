cmake_minimum_required(VERSION 3.22)
project(qtrade_public_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(qtrade_common REQUIRED CONFIG)
find_package(qtrade_engine REQUIRED CONFIG)

add_executable(qtrade_public_install_consumer public_install_consumer.cpp)
target_link_libraries(qtrade_public_install_consumer PRIVATE
  qtrade_common::qtrade_common
  qtrade_engine::qtrade_engine)
