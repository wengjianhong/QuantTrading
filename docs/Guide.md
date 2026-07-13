# 开发指南

本文档说明**仓库目录约定与研发协作要点**。系统设计目标、模块边界与交互规则以 `[Architecture.md](Architecture.md)` 为权威来源；实现演进时需与该文档同步评审。

---

## 1. 与架构文档的对应关系

|《[Architecture.md](Architecture.md)》层次|代码侧落到何处|
|---|---|
|适配层|`src/qtrade_sdk/`（可插拔动态库：行情源、交易通道协议转换；按厂商分子目录 mock/、emt/）|
|交易引擎层|`src/qtrade/engine/`（单进程封闭运行：事件总线、行情/交易标准化、策略引擎、OMS、EMS、账户、持仓、实时风控、合规）|
|支撑服务客户端|`src/qtrade/client/`（轻量级异步客户端：日志、配置、**账户凭证**、监控、服务发现）|
|支撑服务层|`src/qtrade/service/<名称>/`（独立进程 / 镜像部署；与引擎经 gRPC 控制面与 `client/` 旁路接口交互）|
|内部框架基建|`src/qtrade/framework/`（gRPC、数据库 bootstrap、支撑服务生命周期等共享实现；**不 install**）|
|表级 DAO|`src/qtrade/dao/`（`trading_account`、`account_credential`、`engine_config` 等；接口见 `include/qtrade/dao/`）|
|接入层（外部独立项目）|不在本仓库；北向 HTTP REST，南向调 QTrade 支撑服务 gRPC|见《架构》§五|
|企业级治理|多与**外部接入层**、运维流水线耦合：`release/deploy/`、`release/ci/`；Keycloak 等 IdP 可外置|

**性能口径**：A/B/C/D 链路分段、控制面/数据面边界见《架构》**§1.3、§2.1**；**A 段**（策略回调同线程）禁止同步阻塞远程服务、禁止等待磁盘 fsync。

---

## 2. 代码结构（目标布局与当前仓库）

仓库根目录名可与克隆方式一致（如 `qtrade/`）；下列树状结构与《架构》**第三节～第七节**对齐，完全匹配当前代码布局。

```shell
qtrade/
├── CMakeLists.txt                  # 根构建入口：全局选项、依赖、include(cmake/...)
├── cmake/                          # 构建脚本（qtrade_ 前缀 + snake_case，与 src/ 分离）
│   ├── qtrade_paths.cmake          # 工程路径变量
│   ├── lib/qtrade_common.cmake     # 库：qtrade_common
│   ├── lib/qtrade_core.cmake       # 库：qtrade_core
│   ├── app/qtrade_engine.cmake     # 可执行文件 qtrade_engine
│   ├── app/qtrade_services.cmake   # include 各支撑服务
│   └── app/services/qtrade_*.cmake # 每个微服务独立构建脚本
├── docs/
│   ├── architecture.md             # 系统架构权威文档：架构总览、模块设计、交互方式、容灾方案、安全合规
│   ├── guide.md                    # 开发指南：编码规范、插件开发流程、部署步骤、调试方法、协作规则
│   └── deploy.md                   # 部署文档：各模块部署要求、机器配置、网络拓扑、监控告警、容灾切换
├── include/
│   ├── qtrade/                 # 【对外公共头文件】插件接口、共享数据结构、错误码
│   │   ├── structs/            # 框架通用结构（如 result.hpp）
│   │   ├── error_code/         # 错误码：error_codes.hpp、code_segment.hpp、code_message.hpp
│   │   ├── strategy/           # 策略基类接口：IStrategy
│   │   └── dao/                # DAO 接口声明（ddl.hpp / dml.hpp）；表实现头在 src/qtrade/dao/
│   ├── qtrade_sdk/             # 插件 Target 接口：quote/、trader/（Api + Spi）
│   └── qtrade_framework/       # 【内部框架头文件】不 install；#include <qtrade_framework/...>
│       ├── grpc/               # GrpcHandlerInterface、grpc_status_utils 等
│       └── support/            # 支撑服务生命周期接口（ISupportService）
├── src/
│   ├── qtrade/                     # 【交易平台产品实现】
│   │   ├── apps/                   # 【可部署二进制入口】仅含 main，目录名 = 产物名
│   │   │   ├── qtrade_engine/main.cpp
│   │   │   ├── qtrade_config_service/main.cpp
│   │   │   └── ...
│   │   ├── engine/                 # 【核心交易引擎层】库代码，无 main
│   │   │   ├── event_bus/
│   │   │   ├── normalizer/
│   │   │   ├── strategy/
│   │   │   ├── cms/
│   │   │   ├── ems/
│   │   │   ├── oms/
│   │   │   ├── account/
│   │   │   ├── position/
│   │   │   └── risk/
│   │   ├── client/                 # 【引擎内部】支撑服务出站客户端
│   │   │   ├── common/
│   │   │   ├── log_client/
│   │   │   ├── config_client/
│   │   │   ├── account_client/
│   │   │   ├── monitor_client/
│   │   │   └── registry_client/
│   │   ├── service/                # 【支撑服务层】业务实现（无 main；入口在 apps/）
│   │   │   ├── config_service/     # 异步 gRPC（含 SubscribeConfig Streaming）
│   │   │   │   ├── grpc/           # Async + CQ 接入、CallTag 调度
│   │   │   │   └── ...
│   │   │   ├── account_service/    # 同步 gRPC（Unary RPC）
│   │   │   │   ├── grpc/           # 薄路由层：DatabaseReady → handler.Run()
│   │   │   │   ├── handler/        # 每 RPC 一个 Handler（继承 GrpcHandlerInterface）
│   │   │   │   └── logic/          # 可复用业务工具（converter、codec）
│   │   │   ├── log_service/
│   │   │   └── ...
│   │   ├── dao/                    # 【表级 DAO 实现】.hpp + .cpp（命名空间 qtrade::framework::dao）
│   │   ├── framework/              # 【内部框架实现】实现头在 src；公开接口头在 include/qtrade_framework/
│   │   │   ├── support/            # SupportSyncServiceImpl / SupportAsyncServiceImpl
│   │   │   ├── database/           # 连接选项、DbConnectionHolder、bootstrap
│   │   │   ├── dao/                # dml_utils、ddl_utils、sql_utils（DAO 基建，非表级 DAO）
│   │   │   └── grpc/               # 同步/异步 gRPC Server、CallTag、CQ 循环
│   │   ├── common/                 # 【产品进程公共能力】引擎与各微服务入口共用
│   │   │   ├── app/                # 进程 bootstrap：参数解析、信号处理、服务入口
│   │   │   ├── logging/            # 日志初始化
│   │   │   └── proto/              # proto ↔ JSON 等通用转换
│   │   └── error_code/             # include/qtrade/error_code/ 的实现（目录镜像）
│   └── qtrade_sdk/                 # 【SDK 接口实现】对应 include/qtrade_sdk/
│       ├── mock/quote|trader/      # Mock 开发/测试适配
│       └── emt/quote|trader/       # EMT 厂商适配
├── config/                         # 【示例配置】与 build/bin 二进制同名（--config 传入）
│   ├── qtrade_engine.json          # 引擎引导：config/account 地址、engine_id、log/monitor
│   ├── qtrade_config_service.json
│   ├── qtrade_account_service.json # 【规划】account-service 进程配置
│   ├── qtrade_log_service.json
│   └── ...                         # 其余 qtrade_*_service.json
├── demo/                           # 【示例代码】仅用于演示，不参与生产部署
│   └── strategy/                   # 可插拔策略插件开发示例：趋势跟踪、套利等简单策略实现
├── test/                           # 【测试代码】按模块分类，单元测试、集成测试、性能测试
│   ├── engine/                     # 交易引擎测试：核心模块单元测试、链路测试
│   ├── quote/                      # 行情适配器测试
│   ├── trader/                     # 交易适配器测试
│   ├── strategy/                   # 策略测试：策略插件单元测试、回测验证
│   ├── client/                     # 客户端测试：日志、配置、监控客户端连通性测试
│   └── service/                    # 支撑服务测试：服务可用性、性能测试
└── release/                        # 【发行与运维】自动化部署、CI/CD、运维脚本
    ├── ci/                         # 自动化流水线：编译、测试、打包、镜像构建脚本
    └── deploy/                     # 部署脚本：分模块部署、启停、滚动更新、容灾切换脚本
```

**说明**：

1. **进程模型**：可执行入口在 **`src/qtrade/apps/`**（仅 `main.cpp`）；服务实现编译为静态库（如 `qtrade_config_service_static`、`qtrade_account_service_static`），供可执行文件与单元测试链接。**构建定义在 `cmake/`**。可执行目标名与安装二进制同名（如 `qtrade_account_service`），与 `_static` 静态库目标区分。

2. **本地运行示例**（`build/bin/`，在项目根目录执行；配置文件与二进制同名）：
   ```shell
   ./build/bin/qtrade_config_service --config config/qtrade_config_service.json
   ./build/bin/qtrade_account_service --config config/qtrade_account_service.json  # 规划
   ./build/bin/qtrade_engine --config config/qtrade_engine.json
   ./build/bin/qtrade_log_service --config config/qtrade_log_service.json
   ./build/bin/qtrade_monitor_service --config config/qtrade_monitor_service.json
   ```
   Ctrl+C 退出支撑服务。

   **配置分层**见《架构》§2.5。

3. 尚未创建的目录（如 `history_market_service/`）可在对应里程碑落地时补齐。**接入层（网关/控制台）为外部独立项目，不在本仓库。**

4. 策略代码**由独立仓库维护**，本仓库仅保留 `demo/strategy/` 作为开发示例；策略独立仓库规范见 **§7.2**。

5. **gRPC 接入模式**（支撑服务）：
   - **Unary-only**（如 `account_service`）：同步 `Service::Service` + `SupportSyncServiceImpl`；`grpc/` 薄路由，`handler/` 按 RPC 继承 `GrpcHandlerInterface`
   - **含 Streaming**（如 `config_service`）：异步 `AsyncService` + CQ + `SupportAsyncServiceImpl`；`grpc/` 负责 CallTag 生命周期，业务逐步下沉到 `handler/`

6. 若日后将通用基础组件（线程池、无锁结构、工具函数等）从 `include/` + `src/` 中独立为 `base/` 目录，保持与《架构》「共享基础代码」职责一致即可。

---

## 3. 热路径与非热路径（开发约束摘要）

### 3.1 A 段热路径（强制约束）

- **定义**（§1.3.1）：Lane-M → 策略 → CMS → Risk → OMS → EMS 入队（完整路径 **§8.2①**）

- **发单主链**（§1.3.4）：A 段结束后 **紧接 C 段**（EMS 出队 → 适配器 → 交易所）；A+C 为完整发单路径，**分开计量 SLA**

- **同线程禁止**：同步阻塞、磁盘 fsync、**等待交易所/远程 ack**、调用支撑服务 client（C 段 RTT 不计入 A 段 50μs）

- **B 段（异步）**：OMS WAL、合规日志、快照；自 OMS 变更**并行**触发，专用 I/O 线程，**不得阻塞 A 段**；不在 A→C 主链中间

- **控制面**：本地快照 + gRPC `SubscribeConfig` 变更推送；冷启动 `GetConfig` 拉全量（独立控制线程/启动阶段，不在策略回调内）

### 3.2 C 段出站与 D 段旁路

- **C 段**：EMS → 执行适配器 → 交易所/券商，**单独 SLA**，与 A 段分别压测（见《架构》§1.3.4）

- **D 段及非热路径**：回测、报表、日志检索、事后审计、批量查询等；Outbound 旁路上报；背压策略见《架构》§8.1（A 段永不因旁路满而阻塞）

---

## 4. 协议与集成要点

### 4.1 通信协议分层

|交互场景|推荐协议|约束|
|---|---|---|
|交易引擎内部|内存结构体 + 无锁队列|无网络、无序列化|
|交易引擎 ↔ 适配器|函数调用 + 回调接口|同进程内|
|交易引擎 → 支撑服务（D 段）|`client/` 异步接口 + Protobuf|Outbound 线程 fire-and-forget；内部传输可插拔，MVP 可 stub|
|引擎 ↔ config-service|gRPC + Protobuf|引擎仅作 Client：`GetConfig` + `SubscribeConfig`（`EngineConfig`）|
|引擎 ↔ account-service|gRPC + Protobuf|【规划】引擎 Client：`ResolveCredential`（启动/换密，不进 A 段）|
|支撑服务之间|gRPC + Protobuf|同步 / 异步均可（如 config 写入前校验 account 授权）|
|接入层 ↔ 外部系统|HTTP(S)/WebSocket|**外部独立项目**；RESTful 北向，网关转 gRPC 调本仓库支撑服务|
|外部接入层 → QTrade 支撑服务|gRPC + Protobuf|config / **account** / history / observability 等（`src/qtrade/service/`）|

### 4.2 旁路上报与配置 Watch 规范

- **D 段**：`log_client`、`monitor_client` 等仅定义引擎侧异步接口；是否实现远程 gRPC/HTTP、是否 no-op 由里程碑决定

- **控制面（config）**：配置变更经 config-service 审计后，由 `SubscribeConfig` 流推送 `ConfigSnapshot`（`version` + `EngineConfig`）；引擎按 `version` 幂等应用

- **凭证面（account）**：登录凭证经 account-service 按需拉取，不进 `SubscribeConfig`（详见《架构》§2.5）

- **优先级**：P0 审计须经本地 Spool 保底，不得仅依赖远程上报


---

## 5. 插件、Protobuf 与版本管理

### 5.1 可插拔插件规范

系统包含三类可插拔组件，均编译为独立动态库（`.so`/`.dll`）：

1. **行情适配器**（Target = `qtrade_sdk::quote::QuoteApi` / `QuoteSpi`）
   - `src/qtrade_sdk/mock/quote/`：`mock_quote_api`、`mock_quote_spi`
   - `src/qtrade_sdk/emt/quote/`：`emt_quote_api`、`emt_quote_spi`

2. **交易适配器**（Target = `qtrade_sdk::trader::TraderApi` / `TraderSpi`）
   - `src/qtrade_sdk/mock/trader/`：`mock_trader_api`、`mock_trader_spi`
   - `src/qtrade_sdk/emt/trader/`：`emt_trader_api`、`emt_trader_spi`

**双向适配约定**（详见 `docs/Architecture.md` §7.0）：

| 适配器 | 继承 | 职责 |
|--------|------|------|
| `XxxQuoteApi` / `XxxTraderApi` | **Target Api**（`qtrade_sdk::*Api`） | 引擎主动调用 → 转发至厂商 Api |
| `XxxQuoteSpi` / `XxxTraderSpi` | **Adaptee Spi**（厂商 `*Spi`，接入 SDK 后） | 厂商回调 → 结构体转换 → 调用引擎注册的 Target `*Spi` |
| 引擎内 `QuoteNormalizer` / `TraderNormalizer` | **Normalizer** | 适配器回调；仅标准化 + `Publish*`（回报业务见 `ReturnHandler`，§3.1） |

Spi 适配器**不**继承 `qtrade_sdk::*Spi`；`#include` 该头文件仅为使用 `QuoteSpi*` 与结构体类型。

3. **策略插件**：继承 `IStrategy` 基类，**由独立仓库维护**，本仓库仅保留示例

**插件约束**：

- 加载前必须进行**签名校验**，防止恶意插件

- ABI 版本与核心二进制严格兼容，发布说明中必须包含兼容矩阵

- 插件运行在沙箱环境中，资源使用受限制，单个插件崩溃不影响核心引擎

### 5.2 Protobuf 接口规范

- 接口演进：以向后兼容方式演进（新增字段、新增方法）；破坏性变更通过新 service/package 版本并行发布

- 命名规范：遵循 Google Protobuf 命名规范，消息名、字段名使用 snake_case

- 版本管理：每个 proto 文件包含版本号，变更时同步更新版本号

---

## 6. 关键设计细节

### 6.1 共享基础代码

- 跨模块共享的数据结构定义在 `qtrade_sdk/quote/`、`qtrade_sdk/trader/`；按需 `#include` 对应头文件，使用 `qtrade_sdk::quote::`、`qtrade_sdk::trader::` 命名空间
- 错误码枚举见 `include/qtrade/error_code/error_codes.hpp`，分段规则见 `code_segment.hpp`
- `include/qtrade/` 下需 `.cpp` 的公共 API 实现，目录镜像放在 `src/qtrade/error_code/`（如 `code_message.cpp`）；SDK 适配器实现在 `src/qtrade_sdk/<vendor>/`；引擎内部 client 头文件与实现均在 `src/qtrade/client/`
- 模块内部头文件与 `.cpp` 同目录放在 `src/` 下，不放入 `include/`；**`src/` 内部引用**统一以 `src/` 为 include 根，路径带层前缀，例如：
  - `#include "qtrade/service/account_service/account_service.hpp"`
  - `#include <qtrade_framework/grpc/grpc_handler_interface.hpp>`（`include/qtrade_framework/`，不 install）
  - `#include "qtrade/dao/trading_account.hpp"`
  - `#include "qtrade_sdk/mock/quote/mock_quote_api.hpp"`
  （CMake 对实现库使用 `target_include_directories(... PRIVATE ${QTRADE_SRC_DIR})`；公共头使用 `${QTRADE_INCLUDE_DIR}`）
- **Handler 管道内业务数据（ServerData）**使用 DAO 记录或内部 struct，**不直接持有 proto**；proto ↔ 内部结构在 `ConvertToServerData` / `BuildResponse` 边界转换（参考 `account_service/handler/`）

- 通用工具函数（时间、字符串、加密等）统一放在 `include/common/utils/`

- 核心常量（最大订单量、默认超时时间等）统一放在 `include/common/constants.h`

### 6.2 交易引擎封闭性

- 交易引擎**不对外开放任何 TCP/HTTP/gRPC 控制服务端**，所有外部操作只能通过支撑服务间接进行

- 策略**仅由内部行情 Tick/Bar 事件驱动**，不接受任何外部触发信号

- 所有**引擎业务配置**更新经 config-service 的 gRPC `SubscribeConfig` 推送；**交易凭证**经 account-service 单独管理（《架构》§2.5）；禁止外部直接修改交易引擎内存

### 6.3 可观测性

- 热路径埋点采用低开销实现，避免破坏微秒级延迟预算

- 指标与 Trace 采用异步导出 + 采样机制，采样率可动态调整

- 核心交易日志独立存储，保存期限≥3 年，不可篡改

---

## 7. 仓库演进与多仓拆分

### 7.1 主仓拆分原则

当微服务数量或团队边界扩大时，按以下优先级拆分：

1. 优先拆分**成熟、接口稳定**的通用服务（如日志、监控、备份）

2. 其次拆分**业务独立**的服务（如回测、审计、策略研发）

3. **交易引擎层**与强依赖适配器保留在主仓，缩短迭代路径

4. 拆分后依赖通过**内部制品库**管理，避免 git submodule 的复杂性

### 7.2 策略独立仓库规范

策略代码由独立仓库维护，与主仓完全解耦：

- **仓库结构**：每个策略或策略组一个独立仓库，包含策略代码、测试用例、配置文件

- **版本管理**：策略版本与主仓引擎版本兼容，发布说明中明确兼容的引擎版本范围

- **编译部署**：策略编译为独立动态库，通过策略研发服务上传、测试、灰度发布

- **权限控制**：细粒度权限管理，不同团队 / 交易员只能访问自己的策略仓库

### 7.3 协作流程

1. 核心引擎变更：提交 PR → 代码评审 → 合并到主分支 → 自动构建 → 发布版本

2. 策略变更：策略仓库提交 PR → 代码评审 → 自动编译 → 沙箱测试 → 灰度发布 → 全量发布

3. 支撑服务变更：提交 PR → 代码评审 → 合并到主分支 → 自动构建 → 滚动更新

---

## 8. 编码规范

- 所有代码严格遵循 **Google C++ 命名规范**

- 目录名、文件名：小写字母 + 下划线

- 类型名（类、结构体、枚举）：每个单词首字母大写，无下划线

- 变量名、函数参数：小写字母 + 下划线

- 成员变量：小写字母 + 下划线，末尾加下划线

- 常量名：大写 + 下划线

- 函数名：每个单词首字母大写，无下划线

- 命名空间：小写字母 + 下划线

---

## 9. 异常处理与错误码

- 核心交易引擎内部使用**错误码**而非异常，避免异常带来的性能开销

- 支撑服务可使用异常，但必须在边界处捕获并转换为错误码返回

- 所有错误码统一在 `include/qtrade/error_code/` 中定义，采用 **AABBBCCC** 三级编码：AA=10（qtrade 系统级）、BBB=模块编号（engine/service 等从 001 起）、CCC=具体错误码

- 错误信息必须清晰、具体，便于问题定位

---

## 10. 测试要求

- 核心交易引擎模块单元测试覆盖率≥90%

- 适配器模块单元测试覆盖率≥80%

- 支撑服务单元测试覆盖率≥70%

- 所有核心流程必须有集成测试覆盖

- 每次提交必须通过单元测试，合并到主分支前必须通过所有测试

