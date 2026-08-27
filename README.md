# qtrade_engine

License: CC BY-NC-SA 4.0（禁止商用，仅供学习研究）

高性能 C++ 量化交易核心库：对外提供 `IEngine`、SDK SPI 与桥接接口。

**不产出交易进程或策略 `.so`。** 进程入口见 **qtrade_client**；策略插件见 **qtrade_strategy**；支撑微服务见 **qtrade_service**。

安装前缀固定为目录隔离 `/usr/local/qtrade`。发布包只含稳定契约（engine、bridge、sdk、strategy）；错误码与 `Result<T>` 由 **qtrade_common** 提供。引擎实现头不安装。

## 项目关系

```text
cpputils → qtrade_common → qtrade_engine（本仓，libqtrade_engine.so）
                               ├── qtrade_service     微服务实现依赖引擎公开契约
                               ├── qtrade_strategy    策略实现 IStrategy / ABI
                               └── qtrade_client      CreateEngine() + 注入适配器与桥
```

## 环境要求

- C++20 及以上
- CMake 3.22+
- 已安装 cpputils（`/usr/local/cpputils`）、qtrade_common（`/usr/local/qtrade`）
- spdlog、nlohmann_json
- 操作系统：Linux（推荐 Ubuntu 24.04+）

## 编译安装

须先安装 cpputils 与 qtrade_common。

```shell
cmake -B build \
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
├── include/qtrade/          # sdk / engine / bridge / strategy
└── config/qtrade_engine.json
```

## 下游项目使用

```cmake
find_package(qtrade_engine CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE qtrade_engine::qtrade_engine)
```

configure 额外参数：`-DCMAKE_PREFIX_PATH="/usr/local/cpputils;/usr/local/qtrade"`。

单测：

```shell
cmake --build build --target qtrade_tests -j1
./build/test/bin/qtrade_tests
```

## 运行

库本身不能单独跑交易。本地联调顺序：

1. 启动 **qtrade_service** 三个微服务（见该仓 README）
2. 安装 **qtrade_strategy** 插件
3. 运行 **qtrade_client**：`qtrade_client --config /usr/local/qtrade/config/qtrade_engine.json`

设计见 [Architecture.md](docs/Architecture.md)，目录约定见 [Guide.md](docs/Guide.md)。
