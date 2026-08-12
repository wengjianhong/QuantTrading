# 开发指南

本文档说明**仓库目录约定与研发协作要点**。系统设计目标、模块边界与交互规则以 `[Architecture.md](Architecture.md)` 为权威来源；实现演进时需与该文档同步评审。

---

## 1. 与架构文档的对应关系

|《[Architecture.md](Architecture.md)》层次|代码侧落到何处|
|---|---|
|适配层|`src/qtrade_sdk/`（可插拔动态库：行情源、交易通道协议转换；按厂商分子目录 mock/、emt/）|
|交易引擎层|`src/qtrade_engine/`（与 `common/` 同级的扁平模块：事件总线、OMS、EMS、账户、持仓、风控、合规等；`#include` 仍为 `qtrade/engine/...`）|
|支撑服务客户端|见 **qtrade_service** 仓库 `src/qtrade_service/client/`|
|支撑服务层|见 **qtrade_service** 仓库 `src/qtrade_service/service/<名称>/`|
|内部框架基建|见 **qtrade_service** 仓库 `src/qtrade_service/framework/`|
|表级 DAO|见 **qtrade_service** 仓库 `src/qtrade_service/dao/<service>/`|
|接入层（外部独立项目）|不在本仓库；北向 HTTP REST，南向调 QTrade 支撑服务 gRPC|见《架构》§五|
|外部企业基础服务|由机构平台提供；QTrade 仅集成身份、数据安全、运维和合规能力，不负责其实现或部署|

**性能口径**：A/E/C/B/D 链路分段、控制面/数据面边界见《架构》**§2.1、§2.2**；A 段止于 `OrderIntent` 入队，禁止同步远程服务和磁盘 I/O。当前不启用订单主日志 J 段。

---

## 2. 代码结构（目标布局与当前仓库）

仓库根目录名可与克隆方式一致（如 `qtrade/`）；下列树状结构与《架构》**第三节～第六节**对齐，完全匹配当前代码布局。

```shell
qtrade/
├── CMakeLists.txt                  # 根构建入口：全局选项、依赖、include(cmake/...)
├── cmake/                          # 构建脚本（qtrade_engine_ 前缀，与 src/ 分离）
│   ├── qtrade_engine_paths.cmake   # 工程路径变量
│   ├── qtrade_engine_lib.cmake     # 共享库 libqtrade_engine.so
│   ├── qtrade_engine_install.cmake # install / find_package 导出
│   └── qtrade_engine-config.cmake  # 包配置入口

├── docs/
│   ├── architecture.md             # 系统架构权威文档：架构总览、模块设计、交互方式、容灾方案、安全合规
│   ├── guide.md                    # 开发指南：编码规范、插件开发流程、部署步骤、调试方法、协作规则
│   └── deploy.md                   # 部署文档：各模块部署要求、机器配置、网络拓扑、监控告警、容灾切换
├── include/
│   ├── qtrade/                 # 【对外公共头文件】插件接口、共享数据结构、错误码
│   │   ├── structs/            # 框架通用结构（如 result.hpp）
│   │   ├── error_code/         # 错误码：error_codes.hpp、code_segment.hpp、code_message.hpp
│   │   ├── grpc/               # gRPC Handler 与状态映射工具
│   │   ├── strategy/           # 策略基类接口：IStrategy
│   │   ├── support/            # 支撑服务生命周期接口（ISupportService）
│   │   └── dao/                # DAO 接口声明（实现见 qtrade_service）
│   ├── qtrade_sdk/             # 插件 Target 接口：quote/、trader/（Api + Spi）
├── src/
│   ├── qtrade/                     # 【交易平台产品实现】
│   │   ├── apps/                   # 【可部署二进制入口】仅含 main，目录名 = 产物名
│   │   │   ├── qtrade_engine/main.cpp
│   │   │   ├── qtrade_config_service/main.cpp
│   │   │   └── ...
│   │   ├── engine/                 # 【核心交易引擎层】库代码，无 main
│   │   │   ├── core/               # 启动编排、生命周期与订单流水线
│   │   │   ├── event_bus/
│   │   │   │   ├── quote_event_reactor.*   # Lane-Q：Tick/Bar 事件
│   │   │   │   └── trader_event_reactor.*  # Lane-T：Order/Trade 回报事件
│   │   │   ├── core/               # 启动编排、生命周期、订单流水线、行情健康监控
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
│   │   │   ├── config_service/     # gRPC（引擎路径：GetEngineConfig；Subscribe 可保留未用）
│   │   │   │   ├── grpc/           # Async + CQ 接入、CallTag 调度
│   │   │   │   └── ...
│   │   │   ├── account_service/    # 同步 gRPC（Unary RPC）
│   │   │   │   ├── grpc/           # 薄路由层：DatabaseReady → handler.Run()
│   │   │   │   ├── handler/        # 每 RPC 一个 Handler（继承 GrpcHandlerInterface）
│   │   │   │   └── logic/          # 可复用业务工具（converter、codec）
│   │   │   └── ...
│   │   ├── dao/                    # 【表级 DAO 实现】按服务分子目录（命名空间 qtrade::framework::dao）
│   │   │   ├── account_service/    # trading_account、account_credential
│   │   │   ├── config_service/
│   │   │   │   ├── engine/          # engine_config
│   │   │   │   └── risk/            # A 段风险策略（含 instance_risk_policy）
│   │   │   ├── account_risk_service/ # E 段账户硬限制 policy/ledger/reservation
│   │   │   └── risk_tables.hpp     # 风控表总览 include
│   │   ├── framework/              # 【内部框架实现】实现头在 src；公开接口头在 include/qtrade/
│   │   │   ├── support/            # SupportSyncServiceImpl / SupportAsyncServiceImpl
│   │   │   ├── database/           # 连接选项、DbConnectionHolder、bootstrap
│   │   │   ├── dao/                # dml_utils、ddl_utils、sql_utils（DAO 基建，非表级 DAO）
│   │   │   ├── grpc/               # gRPC 传输层
│   │   │   │   ├── sync/           # GrpcSyncServer
│   │   │   │   └── async/          # GrpcAsyncServer、CallTag、CQ 循环
│   │   │   └── error_code/         # include/qtrade/error_code/ 的实现（如 code_message.cpp）
│   └── qtrade_sdk/                 # 【SDK 接口实现】对应 include/qtrade_sdk/
│       ├── mock/quote|trader/      # Mock 开发/测试适配
│       └── emt/quote|trader/       # EMT 厂商适配
├── config/                         # 【示例配置】与 build/bin 二进制同名（--config 传入）
│   ├── qtrade_engine.json          # 引擎引导：config/account 地址、engine_id、log/monitor
│   ├── qtrade_config_service.json
│   ├── qtrade_account_service.json # 【规划】account-service 进程配置
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
   ./build/bin/qtrade_monitor_service --config config/qtrade_monitor_service.json
   ```
   Ctrl+C 退出支撑服务。

   **配置分层**见《架构》§2.6。

3. 尚未创建的目录（如 `history_market_service/`）可在对应里程碑落地时补齐。**接入层（网关/控制台）为外部独立项目，不在本仓库。**

4. 策略代码**由独立仓库维护**，本仓库仅保留 `demo_strategy/` 作为开发示例（每策略一目录）；策略独立仓库规范见 **§7.2**。

5. **gRPC 接入模式**（支撑服务）：
   - **Unary-only**（如 `account_service`）：同步 `Service::Service` + `SupportSyncServiceImpl`；`grpc/` 薄路由，`handler/` 按 RPC 继承 `GrpcHandlerInterface`
   - **含 Streaming**（如 `config_service`）：异步 `AsyncService` + CQ + `SupportAsyncServiceImpl`；`grpc/` 负责 CallTag 生命周期，业务逐步下沉到 `handler/`

6. 若日后将通用基础组件（线程池、无锁结构、工具函数等）从 `include/` + `src/` 中独立为 `base/` 目录，保持与《架构》「共享基础代码」职责一致即可。

---

## 3. 热路径与非热路径（开发约束摘要）

### 3.1 A 段热路径（强制约束）

- **定义**（§2.1.1）：Lane-Q → 策略 → CMS → Risk → `OrderIntent` 入队

- **发单主链**（§2.1.4）：A → [E] → OMS(内存) → C。Production/Institutional 强制执行 E 段；发单前不做订单主日志落盘；无独立 J 段提交门槛

- **同线程禁止**：同步阻塞、磁盘 I/O、等待交易所/远程 ACK、调用支撑服务 client

- **OMS（内存）**：进程内订单状态机；冷启动为空；崩溃后以柜台快照 Adopt 重建 Working 态并对 account-risk 预占对账；**禁止**按本地旧意图自动补单。spdlog/运行日志 ≠ 恢复事实源

- **B/D 段（异步）**：内存快照、历史副本、指标和远程审计投递；旁路 spool（若启用）不是订单恢复源，不得另建一套订单事实

- **控制面**：进程入口（client）启动时出站 `GetEngineConfig`，经 `SetEngineConfig` 值注入引擎；**不**订阅运行时推送，配置变更须停引擎后重 Init/Start

### 3.2 E/C 段与旁路

- **E 段**：同步 `ReserveOrder`；超时或断连为 `Unknown`，必须使用同一 `order_id` 查询，不能直接当拒绝

- **C 段**：OMS 内存受理后 → EMS → 执行适配器 → 交易所/券商。明确确认请求未提交时，使用同一 `order_id` 有界重试；不可重试或次数耗尽则记录 `SendFailed` 并尽力释放预占。发送结果未知时先查询，禁止盲目重发

- **D 段及非热路径**：回测、报表、日志检索、事后审计、批量查询等；Outbound 旁路上报；背压策略见《架构》§7.1（A 段永不因旁路满而阻塞）

> **说明**：原「J 段（订单主日志）」当前不启用；订单主日志为后续可选能力。
---

## 4. 协议与集成要点

### 4.1 通信协议分层

|交互场景|推荐协议|约束|
|---|---|---|
|交易引擎内部|内存结构体 + Lane-Q/Lane-T 内存队列|无网络、无序列化|
|交易引擎 ↔ 适配器|函数调用 + 回调接口|同进程内|
|交易引擎 → 支撑服务（D 段）|`client/` 异步接口 + Protobuf|Outbound 线程 fire-and-forget；内部传输可插拔，MVP 可 stub|
|引擎 ↔ config-service|gRPC + Protobuf|**client** 出站 `GetEngineConfig` 后 `SetEngineConfig` 注入引擎；引擎不持配置 gRPC；无运行时 Subscribe|
|引擎 ↔ account-service|gRPC + Protobuf|引擎 Client：`GetCredential(account_id, engine_id)`（启动拉取，不进 A 段）|
|引擎 ↔ account-risk-service|gRPC + Protobuf|E 段仅同步 `ReserveOrder`；`Unknown` 查询确认；Release/Settle 为直接 gRPC 尽力调用（失败 warn，靠 TTL/对账）|
|引擎 ↔ safety-control|引擎主动建立双向 gRPC 流|冻结 → 撤单 → 确认/对账 → 必要时断开；返回分阶段 ACK|
|支撑服务之间|gRPC + Protobuf|同步 / 异步均可（如 config 写入前校验 account 授权）|
|接入层 ↔ 外部系统|HTTP(S)/WebSocket|**外部独立项目**；RESTful 北向，网关转 gRPC 调本仓库支撑服务|
|外部接入层 → QTrade 支撑服务|gRPC + Protobuf|config / **account** / history / observability 等（`src/qtrade/service/`）|

### 4.2 旁路上报与配置规范

- **D 段**：`log_client`、`monitor_client` 等仅定义引擎侧异步接口；是否实现远程 gRPC/HTTP、是否 no-op 由里程碑决定

- **控制面（config）**：client 冷启动 `GetEngineConfig` 后 `SetEngineConfig`；策略经 `AddStrategy(config, plugin_so_path)` 登记。**不**做运行时 Subscribe/Watch。变更须停引擎后重 Init/Start

- **凭证面（account）**：登录凭证经 account-service `GetCredential` 按需拉取，与配置面分离（详见《架构》§2.6）；账户以全局唯一 `account_id` 标识

- **P0 事实源**：当前阶段订单恢复以柜台快照 + account-risk 预占对账为准，不以本地订单主日志为准（当前不启用）。本地 Spool（若启用）是远程审计投递缓冲，**不是**订单恢复源，不得维护另一套订单状态，也不得用于自动补单

### 4.3 引擎配置与多实例示例

本地引导配置仅描述进程身份和服务地址；策略、行情源与参数由 config-service 下发。当前仓库的引导样例为 `config/qtrade_engine.json`：

```json
{
  "config_service": "127.0.0.1:50051",
  "account_service": "127.0.0.1:50052",
  "tenant_id": "default",
  "engine_id": "engine-1",
  "log_topic": "qtrade_engine",
  "monitor_endpoint": "stub://local"
}
```

业务配置由 config-service 下发。冷启动拉取失败时，只允许使用签名正确且未过期的本地快照；快照过期后冻结新单，但继续处理回报和撤单。MVP 中一个账户只绑定一个 Active 引擎；同账户同品种的多个模型应合并为组合策略，不通过拆实例实现。

```json
{
  "engine_id": "engine-03",
  "quote_source": "emt-primary",
  "quote_failover": "emt-backup",
  "strategies": [{
    "strategy_id": "mean_reversion_01",
    "strategy_name": "libmean_reversion.so",
    "enabled": true,
    "instruments": ["IF2506", "IH2506"],
    "order_volume": 1,
    "max_position_volume": 2,
    "order_cooldown_ms": 1000,
    "window_size": 20,
    "order_threshold": 0.02,
    "stop_loss_percent": 0.01,
    "take_profit_percent": 0.02
  }]
}
```

业务配置在启动时按 `version` 校验后载入。策略参数与绑定在运行中**不可热更新**；启停粒度为**整交易引擎**（进程 `Start`/`Stop`），不支持单策略运行时启停。策略二进制、品种归属和账户绑定变更须停机后重 Init/Start；MVP 不支持跨实例在线迁移。


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

**双向适配约定**（详见 `docs/Architecture.md` §6.1）：

| 适配器 | 继承 | 职责 |
|--------|------|------|
| `XxxQuoteApi` / `XxxTraderApi` | **Target Api**（`qtrade_sdk::*Api`） | 引擎主动调用 → 转发至厂商 Api |
| `XxxQuoteSpi` / `XxxTraderSpi` | **Adaptee Spi**（厂商 `*Spi`，接入 SDK 后） | 厂商回调 → 结构体转换 → 调用引擎注册的 Target `*Spi` |
| 引擎内 `TradingEngine` + `QuoteHealthMonitor` | **Engine Ingress** | 适配器回调接线至 EventBus；行情健康驱动 READY 门禁 |

Spi 适配器**不**继承 `qtrade_sdk::*Spi`；`#include` 该头文件仅为使用 `QuoteSpi*` 与结构体类型。

一个厂商接入应按以下模式接线；实际类型和回调签名以厂商 SDK 与公共头文件为准：

```cpp
class VendorQuoteApi final : public qtrade_sdk::quote::QuoteApi {
 public:
  void RegisterSpi(qtrade_sdk::quote::QuoteSpi& spi) override {
    vendor_spi_.SetTarget(&spi);
    vendor_api_->RegisterSpi(&vendor_spi_);
  }
 private:
  VendorQuoteSpi vendor_spi_;
  VendorSdk::QuoteApi* vendor_api_;
};

class VendorQuoteSpi final : public VendorSdk::QuoteSpi {
 public:
  void OnMarketData(const VendorSdk::MarketData& data) override {
    target_->OnDepthMarketData(Normalize(data));
  }
 private:
  qtrade_sdk::quote::QuoteSpi* target_{};
};
```

Api 适配器实现 QTrade 的稳定接口并转发调用；Spi 适配器继承厂商回调接口、转换数据后由 `TradingEngine` 接线发布至 EventBus。不得在适配器内执行策略、风险裁决或 OMS 状态变更。

3. **策略插件**：继承 `IStrategy` 基类，**由独立仓库维护**，本仓库仅保留示例

**插件约束**：

- 加载前必须进行**签名校验**，防止恶意插件

- ABI 版本与核心二进制严格兼容，发布说明中必须包含兼容矩阵

- 策略插件经沙箱编译、签名校验和资源限制后加载；同进程运行不构成恶意代码的强隔离，未捕获异常的策略由引擎熔断

### 5.2 Protobuf 接口规范

- 接口演进：以向后兼容方式演进（新增字段、新增方法）；破坏性变更通过新 service/package 版本并行发布

- 命名规范：遵循 Google Protobuf 命名规范，消息名、字段名使用 snake_case

- 版本管理：每个 proto 文件包含版本号，变更时同步更新版本号

---

## 6. 关键设计细节

### 6.1 共享基础代码

- 跨模块共享的数据结构定义在 `qtrade_sdk/quote/`、`qtrade_sdk/trader/`；按需 `#include` 对应头文件，使用 `qtrade_sdk::quote::`、`qtrade_sdk::trader::` 命名空间
- 错误码枚举见 `include/qtrade/error_code/error_codes.hpp`，分段规则见 `code_segment.hpp`
- `include/qtrade/` 下需 `.cpp` 的公共 API 实现，目录镜像放在 `src/qtrade/framework/error_code/`（如 `code_message.cpp`）；SDK 适配器实现在 `src/qtrade_sdk/<vendor>/`；引擎内部 client 头文件与实现均在 `src/qtrade/client/`
- 模块内部头文件与 `.cpp` 同目录放在 `src/` 下，不放入 `include/`；**`src/` 内部引用**统一以 `src/` 为 include 根，路径带层前缀，例如：
  - `#include "qtrade/service/account_service/account_service.hpp"`
  - `#include <qtrade/grpc/grpc_handler_interface.hpp>`（`include/qtrade/`，随公共头安装）
  - `#include "qtrade/dao/account_service/trading_account.hpp"`
  - `#include "qtrade_sdk/mock/quote/mock_quote_api.hpp"`
  （CMake 对实现库使用 `target_include_directories(... PRIVATE ${QTRADE_SRC_DIR})`；公共头使用 `${QTRADE_INCLUDE_DIR}`）
- **Handler 管道内业务数据（ServerData）**使用 DAO 记录或内部 struct，**不直接持有 proto**；proto ↔ 内部结构在 `ConvertToServerData` / `BuildResponse` 边界转换（参考 `account_service/handler/`）

- 通用工具函数（时间、字符串、加密等）统一放在 `include/common/utils/`

- 核心常量（最大订单量、默认超时时间等）统一放在 `include/common/constants.h`

### 6.2 交易引擎封闭性

- 交易引擎**不对外开放任何 TCP/HTTP/gRPC 控制服务端**，所有外部操作只能通过支撑服务间接进行

- 策略**仅由内部行情 Tick/Bar 事件驱动**，不接受任何外部触发信号

- 引擎业务配置由 client 拉取后经 `SetEngineConfig` 注入；策略插件经 `AddStrategy(config, so路径)` 加载；**交易凭证**经 account-service 单独管理（《架构》§2.6）；禁止外部直接修改交易引擎内存；运行中不接受配置热推

### 6.3 可观测性

- 热路径埋点采用低开销实现，避免破坏微秒级延迟预算

- 指标与 Trace 采用异步导出 + 采样机制，采样率可动态调整

- 核心交易运行日志可旁路上报并归档；保存期限与不可篡改要求按合规策略配置。运行日志 ≠ 订单恢复事实源

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

- 所有错误码统一在 `include/qtrade/error_code/` 中定义，采用 **AAA BBB CCC DDD** 四级编码：AAA=系统、BBB=服务、CCC=模块、DDD=具体错误码；底层布局由 `cpputils/error_code/error_code_layout.hpp` 提供

- 错误信息必须清晰、具体，便于问题定位

---

## 10. 测试要求

- 核心交易引擎模块单元测试覆盖率≥90%

- 适配器模块单元测试覆盖率≥80%

- 支撑服务单元测试覆盖率≥70%

- 所有核心流程必须有集成测试覆盖

- 每次提交必须通过单元测试，合并到主分支前必须通过所有测试

---

## 11. 实现里程碑与当前状态

本节记录仓库实现状态，不构成架构承诺；功能合并或范围调整时应同步更新，并以代码和测试结果为准。

| 架构能力 | 目标阶段 | 当前实现状态 |
|---|---|---|
| EventBus 与双 EventReactor 事件通道 | MVP | ✅ 已有引擎骨架、EventReactorLoop 与事件类型 |
| CMS / OMS / EMS / 风控 / 持仓 | MVP | 🟡 模块骨架已有；内存 OMS 状态机与幂等语义仍待完善；订单主日志后续可选 |
| 配置驱动分片与一品种一策略校验 | MVP | 🟡 `EngineConfig` 模型已对齐；配置校验和策略一对一分发待实现 |
| account-service 与凭证、配置分离 | MVP | ❌ 服务与凭证链路待实现 |
| 行情适配器与 READY 门禁 | MVP | ✅ 已有 `QuoteApi` + `QuoteHealthMonitor`；故障切换待实现 |
| 交易回报标准化与 OMS 串联 | MVP | 🟡 骨架已有；语义标准化与回报链路待完善 |
| config 启动拉取（client Get + SetEngineConfig） | MVP | ✅ client bridge 已接入；引擎无配置 gRPC；无运行时 Watch |
| D 段旁路上报与支撑微服务 | MVP | 🟡 接口或服务桩存在；远程上报与引擎集成待实现 |
| account-risk-service（E 段） | MVP | ❌ 服务、协议与账簿待实现 |
| 外部接入层 | 二期（非本仓库） | 由独立项目实现；本仓库提供稳定 gRPC 契约 |
| 主备与跨机房灾备 | 后续规划 | 当前不纳入实现和验收承诺 |

图例：✅ 可用　🟡 进行中　❌ 未开始

