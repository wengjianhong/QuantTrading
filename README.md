# qtrade_engine

License: CC BY-NC-SA 4.0 (禁止商用，仅供学习研究)

高性能 C++ 量化交易**核心库**：对外提供 `IEngine` / 桥接接口与引擎实现。本仓**不**产出交易进程二进制或策略 `.so`。

- 交易客户端（进程 `qtrade_engine`）：见独立仓库 **qtrade_client**
- 支撑微服务与 gRPC 桥接：见 **qtrade_service**
- 策略插件：见 **qtrade_strategy**

CMake 包名：`qtrade_engine`（`find_package(qtrade_engine)`）。C++ 公开命名空间与头路径仍为 `qtrade::` / `#include <qtrade/...>`；仓内实现目录为 `src/qtrade_engine/`（构建期通过 overlay 映射到 `qtrade/`）。

## 整体设计

- 详见 [Architecture.md](docs/Architecture.md)

## 快速开始

### 环境要求

- C++20 及以上
- CMake 3.22+
- 依赖：[cpputils](https://github.com/wengjianhong/cpputils)、spdlog、nlohmann_json
- 操作系统：Linux（推荐 Ubuntu 24.04+）

**前置步骤**：先按 [cpputils README](https://github.com/wengjianhong/cpputils#编译安装) 安装 cpputils。

### 目录隔离安装（/usr/local/qtrade）

```shell
cmake -B build \
  -DCMAKE_INSTALL_PREFIX=/usr/local/qtrade \
  -DCMAKE_PREFIX_PATH=/usr/local/cpputils
cmake --build build -j1
sudo cmake --install build
```

安装布局：

```
/usr/local/qtrade/
├── lib/libqtrade_engine.a
├── lib/cmake/qtrade_engine/
├── include/qtrade/
├── include/qtrade_sdk/
└── config/qtrade_engine.json   # 引导配置样例
```

### 下游项目依赖

```cmake
find_package(qtrade_engine CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE qtrade_engine::qtrade_engine)
```

若同时依赖 cpputils：`-DCMAKE_PREFIX_PATH="/usr/local/cpputils;/usr/local/qtrade"`。

一键顺序构建（含 service / strategy / client）见 `GitSpace/scripts/cmake-build.sh`。

## 开发指南

- 详见 [Guide.md](docs/Guide.md)
