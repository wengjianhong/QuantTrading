# 量化交易系统(QTrade)

License: CC BY-NC-SA 4.0 (禁止商用，仅供学习研究) 

高性能 C++ 量化交易系统，支持多数据源、多策略和多交易所接入，采用模块化设计与可插拔架构。


## 整体设计

- 详见 [Architecture.md](docs/Architecture.md)


## 快速开始

### 环境要求

- C++20 及以上
- CMake 3.22+
- 依赖：[cpputils](https://github.com/wengjianhong/cpputils)、spdlog、nlohmann_json、gRPC、Protobuf
- 操作系统：Linux（推荐 Ubuntu 24.04+）

**前置步骤**：先按 [cpputils README](https://github.com/wengjianhong/cpputils#编译安装) 安装 cpputils。

安装前缀由用户在 configure / install 时通过 `-DCMAKE_INSTALL_PREFIX` 指定，本仓库 CMakeLists 无需修改。

### 方式一：默认位置安装（/usr/local）

cpputils 已安装到 `/usr/local` 时，qtrade 同样安装到默认前缀。

```shell
git clone git@github.com:wengjianhong/qtrade.git
cd qtrade

# 编译安装
cmake -B build
cmake --build build -j $(($(nproc)/4))
sudo cmake --install build
sudo ldconfig

# 运行
qtrade_config_service --config config/qtrade_config_service.json
```

安装布局：

```
/usr/local/
├── bin/qtrade_*
├── lib/libqtrade_*.a
└── include/qtrade/
```

### 方式二：目录隔离安装（/usr/local/qtrade）

cpputils 已安装到 `/usr/local/cpputils`（见 [cpputils README](https://github.com/wengjianhong/cpputils#方式二目录隔离usrlocalcpputils)）时使用。

```shell
# 编译安装
cmake -B build \
  -DCMAKE_INSTALL_PREFIX=/usr/local/qtrade \
  -DCMAKE_PREFIX_PATH=/usr/local/cpputils
cmake --build build -j $(($(nproc)/4))
sudo cmake --install build

# 配置PATH，注册运行期库可执行文件路径
export PATH=/usr/local/qtrade/bin:$PATH

# 运行
qtrade_config_service --config config/qtrade_config_service.json
```

安装布局：

```
/usr/local/qtrade/
├── bin/qtrade_*
├── lib/libqtrade_*.a
└── include/qtrade/
```

> cpputils 的运行时库路径（`ldconfig`）见 [cpputils README](https://github.com/wengjianhong/cpputils#编译安装)。

### 下游项目依赖 qtrade

```cmake
find_package(qtrade CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE qtrade::qtrade_core)
```

| qtrade 安装方式 | configure 额外参数 |
|----------------|-------------------|
| 方式一（/usr/local） | 无 |
| 方式二（/usr/local/qtrade） | `-DCMAKE_PREFIX_PATH=/usr/local/qtrade` |

若同时依赖 cpputils，cpputils 侧配置见 [cpputils README](https://github.com/wengjianhong/cpputils#下游项目使用)。方式二示例：

```shell
cmake -B build -DCMAKE_PREFIX_PATH="/usr/local/cpputils;/usr/local/qtrade"
```


## 模块说明


## 开发指南

- 详见 [Guide.md](docs/Guide.md)
