# qtrade_engine

License: CC BY-NC-SA 4.0 (禁止商用，仅供学习研究)

高性能 C++ 量化交易**核心库**：对外提供 `IEngine`、SDK SPI 与桥接接口。本仓**不**产出交易进程二进制或策略 `.so`。

- 交易客户端（进程 `qtrade_client`）：见独立仓库 **qtrade_client**
- 跨仓基础能力：见 **qtrade_common**
- 支撑微服务：见 **qtrade_service**
- 策略插件：见 **qtrade_strategy**

CMake 包名：`qtrade_engine`（`find_package(qtrade_engine)`）。C++ 公开命名空间与头路径为 `qtrade::` / `#include <qtrade/...>`。发布包只安装稳定契约（engine、bridge、sdk、strategy）；错误码、`Result<T>` 与基础设施由 **qtrade_common** 提供；引擎实现头不安装。行情/交易适配器（含 mock）在 **qtrade_client**；引擎单测用 `test/stubs/`。

## 整体设计

- 详见 [Architecture.md](docs/Architecture.md)

## 快速开始

### 环境要求

- C++20 及以上
- CMake 3.22+
- 依赖：[cpputils](https://github.com/wengjianhong/cpputils)、spdlog、nlohmann_json、已安装的 `qtrade_common`
- 操作系统：Linux（推荐 Ubuntu 24.04+）

**前置步骤**：先安装 [cpputils](https://github.com/wengjianhong/cpputils#编译安装)，再安装 `qtrade_common`。

### 目录隔离安装（/usr/local/qtrade）

```shell
cmake -S . -B build \
  -DCMAKE_INSTALL_PREFIX=/usr/local/qtrade \
  -DCMAKE_PREFIX_PATH="/usr/local/cpputils;/usr/local/qtrade"
cmake --build build -j1
sudo cmake --install build
```

安装布局：

```
/usr/local/qtrade/
├── lib/libqtrade_engine.so
├── lib/cmake/qtrade_engine/
├── include/qtrade/          # 含 sdk/（QuoteApi/TraderApi 等接口）
└── config/qtrade_engine.json   # 引导配置样例
```

### 下游项目依赖

```cmake
find_package(qtrade_engine CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE qtrade_engine::qtrade_engine)
```

构建时使用：`-DCMAKE_PREFIX_PATH="/usr/local/cpputils;/usr/local/qtrade"`。

一键顺序构建（含 service / strategy / client）见 `GitSpace/scripts/cmake-build.sh`。

## 开发指南

- 详见 [Guide.md](docs/Guide.md)
