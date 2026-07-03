# 量化交易系统架构设计

## 一、系统定位与目标

### 1.1 系统定位

本系统定位为**面向机构客户的企业级多租户量化交易平台**，采用**纯行情驱动、单进程封闭交易引擎**的安全架构，支撑从策略研发、回测、仿真交易到实盘交易的全生命周期管理，适配高频交易场景，满足金融机构合规、高可用、高性能、可扩展、可审计的核心诉求，支持多账户、多交易所、多策略并行运行，具备跨地域部署与容灾能力。

### 1.2 核心目标

- **极致性能**：**核心决策链路**（A 段，§1.3.1：进程内内存决策至 EMS 入队）平均延迟 < 50 微秒；**出站报单**（C 段）与交易所 RTT **单独 SLA**；峰值订单处理能力 ≥ 10 万笔/秒（多实例聚合，§1.3.2）；

- **金融级高可用**：核心交易层全年可用性 ≥ 99.99%，支撑跨机房灾备，故障恢复时间（RTO）< 30 秒，数据恢复点目标（RPO）< 1 分钟；

- **多租户隔离**：支持机构内多部门/多团队隔离，资源（CPU/内存/行情/订单通道）按需分配，数据与操作权限严格隔离；

- **合规可控**：满足金融监管要求（如交易留痕、反洗钱、异常交易监控），支持等保三级、数据脱敏与跨境数据合规；

- **全生命周期可观测**：覆盖指标、日志、链路、调用链全维度监控，支持根因分析与性能溯源；

- **弹性扩展**：核心层按分片水平扩展（每分片 Active-Passive）；支撑服务层秒级弹性伸缩

- **封闭安全**：交易**数据面**仅由内部行情事件驱动发单；**控制面**（策略启停、参数变更、配置下发）经配置中心异步投递，引擎不暴露任何外部 TCP/HTTP 直连控制口，杜绝绕过审计的人为干预；

### 1.3 性能指标口径与热路径定义

为避免验收口径歧义，性能目标按下述**分段计量**，不在单一指标中混写「进程内内存决策」与「持久化/网络/跨服务」耗时。

#### 1.3.1 链路分段（验收边界）

| 分段 | 是否计入 50μs 目标 | 说明 |
|---|---|---|
| **A. 内存决策链** | **是** | 行情入队 → 策略回调 → CMS/风控 → OMS 内存状态更新 → EMS 内存队列入队 |
| **C. 出站发单** | **单独 SLA** | EMS 出队 → 执行适配器 → 交易所/券商（**紧接 A 段**，构成发单主链；见 §1.3.4） |
| **B. 异步持久化** | **否** | OMS WAL、合规日志、内存快照：独立 I/O 线程/批量 fsync；自 OMS 变更**并行分叉**，**不得阻塞 A 段同线程** |
| **D. 旁路上报** | **否** | 经 `log_client`/`monitor_client` 等异步接口上报，单向 fire-and-forget，不等待响应 |

- **「核心决策链路」** 即 **A 段**（进程内内存决策，至 EMS 入队）；**不含** C 段出站报单 RTT。**发单完整路径** = **A → C**（决策 + 报单，§1.3.4）。典型 A 段：标准化行情入队 → 策略回调与信号生成 → CMS 合规校验 → 实时风控 → OMS 内存校验 → EMS 入队。

- **「平均延迟 < 50 微秒」**：**仅 A 段**在稳态、CPU 亲和绑定、无策略复杂计算条件下的 **P99 目标**（非全链路、**不含 C 段 RTT**）；B/C/D 段**分别验收**。

- **「峰值订单处理能力 ≥ 10 万笔/秒」**：多引擎实例按 **租户 + 账户 + 品种** 分片后的**聚合吞吐**；单进程基准需单独压测（见 §1.3.2）。

#### 1.3.2 基准压测场景（单进程）

| 场景 | 延迟目标（P99） | 说明 |
|---|---|---|
| 空策略 passthrough | < 20μs | 仅验证 Lane-M/R + OMS + EMS 骨架 |
| 1 策略 + CMS + 全量风控 | < 50μs | MVP 默认验收场景 |
| N 策略并行 + M 品种 | 单独指标 | 不纳入 50μs 统一承诺，按客户部署压测报告交付 |

#### 1.3.3 控制面与数据面

| 平面 | 驱动来源 | 允许操作 | 禁止操作 |
|---|---|---|---|
| **数据面** | Tick/Bar/订单回报事件 | 策略逻辑、发单/撤单信号、风控拦截 | 外部 HTTP/TCP 直连触发交易 |
| **控制面** | 配置中心 ← gRPC Watch ← 引擎控制线程（出站订阅） | 策略启停/暂停、参数热更新、**实例品种归属**（维护窗口）、行情源地址 | 绕过配置中心直接改引擎内存；同步阻塞热路径 |

控制面变更**不产生订单**，仅改变策略生命周期或配置快照；生效后由下一 Tick 事件自然驱动数据面。

#### 1.3.4 发单主链（A → C）

A 段至 EMS 入队后**紧接** C 段出站（EMS → 适配器 → 交易所），中间不插入 EventBus 二次排队。B 段 WAL 自 OMS 变更**并行**分叉，不挡 A/C。

---

## 二、系统架构总览

![量化系统架构图](./assets/images/Architecture.png "量化系统架构图")
图片地址：https://www.processon.com/v/69ff275de3b4723038f29cb6


### 架构层次：

本系统采用 “**核心模块进程内模块化集成 \+ 支撑功能微服务化 \+ 可插拔实现 \+ 企业级治理与容灾**” 的混合架构，平衡低延迟交易需求与企业级系统的稳定性、扩展性、合规性。

整体分为四层：

- **核心交易层**：**单进程**模块化设计（可多实例部署，§2.4），处理实时交易链路（微秒级响应），Lane-M/R 隔离、本地容灾

- **支撑服务层**：独立部署的微服务集群，提供非实时性支撑功能，强化服务治理、熔断降级、弹性伸缩能力

- **接入层（外部独立项目，非本仓库）**：面向控制台、第三方与运维系统的 **HTTP/REST 北向入口**；统一鉴权、限流、路由至 QTrade **支撑服务**（gRPC）；**不**纳入 `qtrade` 核心仓库（详见 §五）

- **企业级治理层**：跨层治理能力，覆盖服务注册发现、配置灰度、容灾备份、合规审计、可观测性等企业级能力；身份认证等可与接入层或机构 IdP 集成

### 核心设计原则：

- **低延迟优化**：A 段（§1.3.1）全程进程内内存交互；CPU 亲和性绑定，内存锁优化，网卡中断隔离；持久化与旁路上报异步化

- **高可用性**：核心模块多实例部署 \+ 跨机房灾备，微服务集群化 \+ 熔断降级，关键组件主备切换 \+ 故障自动转移

- **可扩展性**：通过插件机制扩展数据源/策略/执行渠道，无需修改核心代码；支撑服务层支持水平扩容，核心层支持**多引擎实例**水平扩展（每实例仍为单进程，见 §2.4）

- **安全性**：多层风控（事前/事中/事后），操作审计，数据加密传输，多租户权限隔离，等保三级合规，敏感数据脱敏

- **可维护性**：模块化边界清晰，微服务独立升级，插件化降低耦合，全维度可观测，自动化运维（发布/回滚/监控）

- **多租户隔离**：资源隔离、数据隔离、权限隔离，租户级配置与策略独立管理

- **合规性**：全流程交易留痕，审计日志不可篡改，满足金融监管与行业合规要求

- **命名规范**：所有目录、文件、模块、接口严格遵循 **Google C\+\+ 命名规范**

- **配置驱动分片**：以**部署层静态配置**（每实例 `engine_id` + 策略及品种绑定）实现分片与扩展（详见 §2.4）

### 2.1 热路径与非热路径划分

- **热路径（A 段，§1.3.1）**：行情解析 → **行情通道（Lane-M）** → 策略触发 → CMS → 风控 → OMS 内存更新 → EMS 入队；**发单主链**紧接 **C 段**出站（EMS → 适配器 → 交易所，§1.3.4）。订单/成交回报走 **Lane-R**（§3.1、§8.2），**优先于 Lane-M**；**A 段同线程禁止**：同步阻塞、调用支撑服务、等待磁盘 fsync、**等待交易所/远程 I/O 响应**（C 段 RTT 单独计量）。

- **准热路径（B 段，异步）**：OMS WAL、合规日志、内存快照；自 OMS 变更**并行分叉**，专用 I/O 线程批量写入，与 A/C 发单主链解耦，**不得阻塞 A 段同线程**。

- **非热路径（D 段旁路）**：日志、监控、历史、审计等；A 段仅写入内存队列，由 **Outbound 线程**调用 `log_client`/`monitor_client` 等**异步上报接口**（Protobuf 载荷），**不等待响应**；client 内部实现（gRPC/HTTP/本地 Spool/stub）**架构不约束**，MVP 允许仅 stub 或本地落盘；支撑服务之间使用 **gRPC** 同步/异步调用。

- **控制面**：配置与风控阈值采用**本地快照缓存 + gRPC 增量订阅**；引擎以 **gRPC Client 出站**连接 config-service：`GetConfig` 冷启动拉全量，`WatchConfig`（Server Streaming）运行时收增量（见 §8.1）；**引擎不对外暴露 gRPC Server**。

- **协议适配器（§7.0，可插拔）**：厂商 API/结构体 → `qtrade_sdk` 字段映射。
- **标准化模块（引擎固定）**：`QuoteNormalizer`/`TraderNormalizer` 做跨柜台语义统一；**不做排队**，队列由 Lane-M/Lane-R 承担。
- **多租户上下文**：Lane-M/Lane-R 事件、OMS 状态、订单 ID 携带 `tenant_id`；策略插件默认不可跨租户访问内存状态；账号/持仓由 OMS 成交路径**同步更新**，不经独立事件总线。

### 2.2 支撑服务分期建设建议

控制开局复杂度，按阶段扩展微服务边界：

- **一期（MVP，合并部署）**：
  - **config-service**：引擎业务配置（`EngineConfig`：策略、品种绑定、行情源、`account_id` 引用）+ gRPC（`GetConfig`/`WatchConfig`）+ 变更审计；**不存储**交易登录密码
  - **account-service（交易账户服务）**：资金账户主数据、登录凭证加密托管、`engine_id` ↔ `account_id` 授权、按需凭证解析（见 §2.5）
  - **observability-service**：日志采集 + 业务监控 + 基础告警（日志/监控原独立服务逻辑保留，可同进程多模块）
  - **history-service（可选）**：历史行情/成交简化存储；回测可暂用本地脚本
  - 引擎：**配置驱动多实例分片**（每实例 `engine_id` + **策略及品种绑定**，见 §2.4）+ **`QuoteApi` 适配器**（经 `QuoteNormalizer` 接入，§7.1）
  - **暂不独立部署**：etcd 服务发现（MVP 可用静态配置）、容灾备份、交易审计

- **二期**：拆分 observability；上线策略管理、交易审计、容灾备份；分片 Active-Passive；可选 Lane-M 内 Tick/Bar 分流、维护窗口改配置调片、多租户资源配额

- **三期**：跨机房 Active-Passive 灾备、监管报送、智能风控；跨机房行情接入点选择

- **原则**：支撑服务与交易引擎**物理隔离**（Lite 档位例外见 §9.1），可按需独立部署/关停，不影响实盘交易

### 2.3 引擎与支撑服务交互模型

本系统**不引入独立消息中间件**（如 Kafka/Redis/NATS）作为引擎与支撑服务的必选基础设施。交互分为两类：

| 类型 | 方向 | 机制 | 说明 |
|---|---|---|---|
| **控制面** | config-service / account-service → engine（逻辑推送或按需拉取） | gRPC（引擎出站 Client） | 引擎**主动出站**订阅 `WatchConfig`；凭证经 `ResolveCredential` **按需**拉取；禁止支撑服务回调引擎 RPC |
| **D 段旁路** | engine → 支撑服务 | `src/client/` 异步上报接口 | Outbound 线程 fire-and-forget；各 client 内部实现可插拔，MVP 可为 stub |
| **冷启动配置** | engine → config-service | gRPC `GetConfig` | 一次性全量拉取 `EngineConfig` + 本地兜底快照 |
| **冷启动凭证** | engine → account-service | gRPC `ResolveCredential` | 启动阶段或换密时按 `account_id` 解析登录材料；**不进** `WatchConfig` 流 |
| **服务间查询** | 支撑服务 ↔ 支撑服务 | gRPC | config 写入前校验账户授权；回测拉历史、审计查询等，不经引擎 |

### 2.4 引擎实例与分片模型（配置驱动，MVP 默认）

水平扩展的基本单元是 **引擎实例**（每个实例 = **一个**单进程封闭引擎），**不是**「一个引擎拆成多进程」。

#### 2.4.1 分片方式

| 方式 | 阶段 | 说明 |
|---|---|---|
| **配置驱动静态分片（默认）** | MVP | 部署 N 个实例；每实例本地引导 JSON（`engine_id` 等）+ config-service（按 `engine_id` 拉取 `EngineConfig`）获取策略及品种绑定、`account_id` 引用；**实例内**每品种最多绑定一个策略（§2.4.3）；行情 Subscribe 取本实例已启用策略 `instruments` 的**并集**（去重） |
| **实例组 Active-Passive** | 二期 | 同一分片配置部署 Active + Standby 两实例；Standby 跟行情/WAL，不发单 |
| **维护窗口调片（可选）** | 二期+ | 运维修改 config 中策略/品种归属，重启或 controlled reload | 品种冷热不均时水平加实例或迁移配置，不做运行时自动 hash |

分片键建议：`tenant_id + account_id + instrument_id`（可按账户级或品种级聚合写入配置）。

#### 2.4.2 与多租户的关系

- **强隔离（推荐）**：一租户 → 一或多个专属实例（配置写死品种范围），运维模型简单，故障域清晰。
- **弱隔离/省资源**：单实例多租户，依赖进程内 `tenant_id` + OMS/策略隔离；config-service 按租户下发策略与风控。
- 无论哪种，**实例之间不共享 OMS/EventBus 内存**；跨实例无全局热路径总线。
- **同账户跨实例**：多个 `engine_id` 可配置相同 `account_id`；实例间**不做**订单/持仓内存协同，**发单是否放行由 CMS + RiskManager 按账户/品种聚合限额统一裁决**（各实例只读本地快照 + 控制面下发的账户级阈值，§2.4.3）。

#### 2.4.3 品种 ↔ 策略绑定约束（MVP 默认）

| 范围 | 规则 | 说明 |
|---|---|---|
| **实例内** | 每个 `instrument_id` **最多绑定一个** `strategy_id` | 一策略可绑定**多个**品种；禁止同实例内两策略共享同一品种 |
| **跨实例** | 同一品种可被**不同** `engine_id` 上的策略绑定 | 多策略同品种需求 → **拆实例**，不做进程内 fan-out 多策略 |
| **配置校验** | config-service 写入/下发前校验 | 同实例 `strategies[]` 的 `instruments` **不得重复**；违规则拒绝并审计 |

**实例内** Tick 到达后：`StrategyEngine` 按品种 **1:1** 投递至唯一策略（无需同 Tick 多策略串行竞争）。**跨实例**同品种、同账户时，各实例独立决策发单，**仅风控层**（及 CMS）在各自 A 段路径上依据账户/品种累计限额拦截或放行。

#### 2.4.4 MVP 部署示例（概念）

同一租户、同一账户下，两个策略均需交易 `IF2506` 时，**必须**拆为两个引擎实例（不可放入同一 `engine_id`）：

**engine-03**（均值回归，IF + IH）：

```json
{
  "engine_id": "engine-03",
  "tenant_id": "tenant_a",
  "account_id": "acc_001",
  "quote_source": "emt-primary",
  "quote_failover": "emt-backup",
  "strategies": [
    {
      "strategy_id": "mean_reversion_01",
      "plugin": "libmean_reversion.so",
      "enabled": true,
      "instruments": ["IF2506", "IH2506"],
      "params": { "lookback": 20, "threshold": 0.02 }
    }
  ]
}
```

**engine-04**（价差，IF + IC；与 engine-03 **共享 account、共享 IF2506 品种，不同策略**）：

```json
{
  "engine_id": "engine-04",
  "tenant_id": "tenant_a",
  "account_id": "acc_001",
  "quote_source": "emt-primary",
  "quote_failover": "emt-backup",
  "strategies": [
    {
      "strategy_id": "spread_ic_if_01",
      "plugin": "libspread.so",
      "enabled": true,
      "instruments": ["IF2506", "IC2506"],
      "params": { "leg_ratio": [1, -1] }
    }
  ]
}
```

上例合法原因：`IF2506` 在 **engine-03 与 engine-04 各出现一次**（跨实例），在**每个实例内**仍满足「一品种一策略」。`QuoteNormalizer` 对各实例分别 Subscribe 并集；`IF2506` Tick 在 engine-03 仅驱动 `mean_reversion_01`，在 engine-04 仅驱动 `spread_ic_if_01`。两实例若同 Tick 均发单，**不在引擎间协调**，由 **CMS/RiskManager** 按 `acc_001` 的限仓、限购、PnL、熔断等规则分别校验并决定是否下单。

config-service 通过 `GetConfig(engine_id)` 返回该实例专属 `EngineConfig`；`WatchConfig` 变更策略启停、参数或品种绑定时，在**维护窗口**重启或 controlled reload；MVP 不做运行时跨实例迁移。交易登录凭证由 **account-service** 单独管理（§2.5），**不**经 `WatchConfig` 下发。

### 2.5 配置与交易账户（account-service）

引擎涉及三类配置/账户，**禁止混用**：

| 概念 | 来源 | 内容 |
|---|---|---|
| **进程引导** | 本地 `qtrade_engine.json` | 连 config/account 服务、`engine_id`、日志/监控；**不含**策略与密码 |
| **业务配置** | config-service → `EngineConfig`（见 §2.4、`config.proto`） | 策略、品种、`account_id` 引用、行情源；**不含**登录密码 |
| **运行时账簿** | 引擎内 `AccountManager` | 可用资金、冻结；**不含**开户与凭证 |

**account-service**（`qtrade_account_service`）单独管理资金账户主数据、加密凭证及 `engine_id` ↔ `account_id` 授权。引擎冷启动或换密时通过 `ResolveCredential` **按需**拉取登录材料；**不进** `WatchConfig` 流，**禁止**写入 `EngineConfig`。

与 config-service 的分工：**config** 管「跑什么策略」；**account** 管「用哪个账户登录」。策略插件只使用 `account_id` 发单，不接触密码。

启动顺序：`qtrade_engine.json` → `GetConfig` → `ResolveCredential` → EMS 登录 → `WatchConfig` 持续收业务配置变更。

---

## 三、交易引擎层（单进程封闭运行，企业级优化）

所有模块运行在同一进程内，通过内存级**双通道事件分发**（Lane-M/Lane-R，§3.1）及 OMS 同步调用通信，避免跨进程延迟。

**数据面**封闭：无对外 TCP/HTTP/gRPC 控制服务端，仅通过适配器对接外部行情与交易所。**控制面**经出站 gRPC 订阅 config-service（§1.3.3、§8.1），不直接触发发单。

|模块名称（Google C\+\+ 规范）|核心功能|可插拔设计|企业级优化点|
|---|---|---|---|
|**事件总线模块**|进程内事件分发，Lane-M/Lane-R 分离|核心基础设施|1. **Lane-R 优先于 Lane-M**；<br />2. Lane-M：Tick/Bar → 策略；Lane-R：`Publish*` → `ReturnHandler` → 策略；<br />3. 控制面不进数据通道；|
|**行情标准化模块（`QuoteNormalizer`）**|跨柜台行情语义统一、校验过滤；标准化后投递 Lane-M（**不自有队列**，排队由 EventBus 承担）|无，引擎固定模块|1. **`QuoteNormalizer` 持有 `QuoteApi`**（`SetQuoteApi`，见 `quote_normalizer.cpp`）；<br />2. 行情校验与异常过滤；<br/>3. `Subscribe` 本实例策略绑定品种的**并集**；<br/>4. 二期可选 failover（换适配器/重连）；<br/>5. failover 断流缓冲在适配器或独立组件，**非**第二套调度队列|
|**交易标准化模块（`TraderNormalizer`）**|跨柜台回报语义统一；标准化后 `Publish*` 至 Lane-R（**不调 OMS**）|无，引擎固定模块|1. 接 `TraderApi` 回报回调（规划）；<br />2. 订单状态、数量单位、错误码映射；<br/>3. **不自有队列**；出站发单：OMS→EMS→适配器|
|**策略引擎模块**|加载、运行、管理策略，生成交易信号|`IStrategy` 插件接口|1. **实例内一品种一策略**（§2.4.3）；Tick/Bar **1:1** 投递；<br />2. **故障隔离**（异常捕获 + 策略级熔断，见 §7.3）；<br />3. 资源限制（订单频率/内存配额）；<br/>4. 控制面热加载/卸载；<br/>5. 异常自动暂停并旁路上报告警|
|**交易合规模块(cms)**|监管/合规硬规则（限仓、限购、日内频次、禁交易名单）|规则可配置化|1. 静态合规规则，变更需审计；<br />2. 合规拦截记录异步落盘（B 段）；<br/>3. 规则经控制面异步更新|
|**订单管理模块(oms)**|订单全生命周期（创建/修改/取消/状态跟踪）|核心流程固定，支持订单类型扩展|1. WAL 异步持久化（B 段），保证不丢单；<br />2. 幂等性（全局唯一 order_id）；<br/>3. 多租户隔离；<br/>4. 超时自动处理|
|**交易执行模块(ems)**|订单路由、拆单、算法执行、通道管理|`qtrade_sdk::trader::TraderApi`/`TraderSpi`（§7.0）|1. 多通道故障转移；<br />2. 流量控制；<br/>3. 拆单算法；<br/>4. 出站发单走 C 段，与 A 段分别计量|
|**账号管理模块（`AccountManager`）**|资金、资产、可用额度**运行时**核算|无，进程内状态|1. 多租户账户隔离；<br />2. 发单/成交路径上 OMS **同步调用**校验与更新（不经 EventBus）；<br/>3. 定时内存快照（B 段异步刷盘）；<br/>4. **与 account-service 区分**：本模块不管登录密码与账户开户，仅维护内存账簿（§2.5）|
|**持仓管理模块(pms)**|持仓、开平、冻结、盈亏实时核算|无，进程内状态|1. 逐标的持仓；<br />2. 今/昨仓区分；<br/>3. OMS 成交回报路径**同步更新**；持仓异常校验|
|**风险管理模块**|实时风险（PnL、亏损、涨跌停、异常行情熔断）|风险规则可配置化|1. 动态风控（租户/账户/策略/品种）；**跨实例同账户**时按账户/品种聚合限额裁决是否发单（§2.4.3）；<br />2. 熔断与降频；<br/>3. 阈值经控制面异步更新|

#### CMS 与 RiskManager 边界

- **调用顺序**：策略信号 → **CMS**（合规硬规则）→ **RiskManager**（实时风险）→ OMS；任一拦截则终止，记录原因并旁路审计。
- **CMS**：偏监管与静态规则，变更频率低，需留痕；**RiskManager**：偏实时 PnL/波动/熔断，阈值可高频调整。
- 两者均只读本地内存快照，**不同步调用**外部服务；**跨实例同账户**时不做实例间实时对账，各实例在 A 段独立校验，依赖控制面下发的**账户级**限额与（二期可选）风控侧聚合视图。

#### 3.1 EventBus 双 Reactor（Lane-M/Lane-R）

为避免行情洪峰阻塞订单回报，进程内事件分发拆为两条 Reactor + 单 FIFO 队列：

| 组件 | 职责 |
|---|---|
| **`EventReactorLoop<Event, Policy>`** | Demultiplex（`cv` + deque）+ `RunOnce()` 出队；`RunLoop` 驱动 Reactor 线程 |
| **`MarketEventReactor`（Lane-M）** | Tick/Bar → `EventReactorLoop<EventPtr>` → 按 `EventType` 调用 EventHandler |
| **`ReturnEventReactor`（Lane-R）** | Order/Trade → `EventReactorLoop<EventPtr>` → 按 `EventType` 调用 EventHandler |
| **`EventLanes`** | EventBus 门面；持有上述两条 Reactor，统一 `Start`/`Stop` |

| 通道 | 事件 | 典型路径 | 线程 |
|---|---|---|---|
| **Lane-M** | Tick、Bar | 适配器 → QuoteNormalizer → `Publish*` → StrategyEngine | `MarketEventReactor` 线程 |
| **Lane-R** | Order、Trade | 适配器 → TraderNormalizer → `Publish*` → ReturnHandler → StrategyEngine | `ReturnEventReactor` 线程 |

- **Reactor 语义**：`Publish` 入队 → Reactor 线程出队 → 按注册顺序调用 Handler。
- **Lane-R**：`ReturnHandler` 调 OMS 并**同步** Account/PMS，**须先于** `StrategyEngine` 注册；`TraderNormalizer` 仅 `Publish*`。
- **发单 / 持久化**：完整路径见 **§8.2**（A→C 主链；B 段并行）。
- MVP 已实现 `MarketEventReactor` + `ReturnEventReactor` + `EventLanes`（§12.1）。

### 核心约束（强制）

1. 所有模块运行在**同一个交易引擎进程**内，不拆分、不独立部署

2. 交易引擎**不对外开放任何 TCP/HTTP/gRPC 控制服务端**；控制面变更由**独立控制线程**经出站 gRPC `WatchConfig` 应用至本地快照

3. **数据面**策略**仅由**内部 Tick/Bar/回报事件驱动，不接受外部触发发单信号

4. **A 段热路径**（§1.3.1）禁止：同步阻塞、等待远程 I/O/磁盘、调用支撑服务；**B/C/D 段**允许异步 I/O 与网络，但不得阻塞 A 段同线程

5. **分片在部署层完成**（§2.4）：每实例仅处理配置内的品种/账户；跨实例迁移仅维护窗口改 config

6. **实例内一品种一策略**（§2.4.3）：同品种多策略必须拆实例；跨实例同账户的发单协调**仅**由 CMS + RiskManager 在各自 A 段路径裁决，引擎间无热路径总线

---

## 四、支撑服务层（独立微服务，企业级治理）

所有服务**独立部署在非交易服务器**（Lite 档位例外见 §9.1），通过 **gRPC（控制面）** 与引擎侧 **`client/` 异步上报接口（D 段旁路）** 与交易引擎通信，强化服务治理、弹性伸缩、容灾备份能力。**不影响交易、不占用交易热路径资源、可随意扩容/缩容/重启**。

|微服务名称|核心功能|技术特性|企业级优化点|
|---|---|---|---|
|**配置中心服务（config-service）**|集中管理**引擎业务配置**（`EngineConfig`：策略及品种绑定、行情源标识、`account_id` 引用、风控阈值等）|支持动态配置更新，配置版本管理，权限控制|1. 配置灰度发布（按模块/**engine_id** 维度）；<br>2. 配置变更审计日志；<br/>3. 配置回滚；<br/>4. 多租户配置隔离；<br/>5. 按 **engine_id** 下发完整 `EngineConfig`，支撑配置驱动分片（§2.4）；<br/>6. **实例内品种绑定唯一性校验**（§2.4.3）；<br/>7. **不存储、不下发**交易登录密码（§2.5）|
|**交易账户服务（account-service）**|资金账户主数据、登录凭证加密托管、`engine_id` ↔ `account_id` 授权、按需凭证解析|独立 gRPC；凭证与业务配置分库分权限|1. 账户 CRUD（元数据，API 不返回明文密码）；<br/>2. 凭证 AES/KMS 加密存储与轮换；<br/>3. `ResolveCredential` 供引擎冷启动登录；<br/>4. 绑定校验（config 写入或引擎解析前校验授权）；<br/>5. 凭证操作独立审计；<br/>6. 二期：短期 lease、账户健康检查、审批流|
|**历史行情服务**|存储/查询历史行情|基于时序数据库（如 ClickHouse），支持高效批量查询，数据自动归档|1. 数据多副本存储（3 副本），跨机房同步；<br/>2. 数据生命周期管理（热数据/冷数据分层存储，冷数据归档至对象存储）；<br/>3. 数据访问权限控制（按租户/品种/时间范围）；<br/>4. 数据脱敏（隐藏敏感账户信息）；<br/>5. 批量导出合规化（带水印/审计日志）|
|**历史交易服务**|存储成交记录|基于时序数据库（如 ClickHouse），支持高效批量查询，数据自动归档|同上|
|**日志分析服务**|收集/存储/检索系统日志与交易流水|支持日志分级、按模块过滤，提供全文检索，日志可视化分析|1. 日志不可篡改（写入后追加模式，哈希校验）；<br/>2. 日志分级存储（核心交易日志独立存储，保存≥3 年）；<br/>3. 多租户日志隔离，支持按租户检索；<br/>4. 日志与调用链关联，支持根因分析；<br/>5. 行情源切换日志独立存储、审计可追溯|
|**业务监控服务**|业务指标（策略收益/订单量）、引擎实例负载（CPU/队列长度/切换次数）|实时指标采集，自定义告警阈值，多渠道通知（邮件/短信/钉钉）|1. 全维度可观测（指标 \+ 日志 \+ 链路 \+ 进程内埋点）；<br/>2. 告警分级（P0\-P4），支持告警升级规则；<br/>3. 租户级监控视图，机构级汇总视图；<br/>4. 性能基线自动生成，异常检测（同比/环比）；<br/>5. 监控数据长期存储（≥1 年），支持趋势分析；<br/>6. 按 **engine_id** 聚合实例负载与行情源切换指标|
|**回测引擎服务**|基于历史数据验证策略有效性|支持多线程并行回测，模拟滑点/手续费，生成绩效报告（夏普比率/最大回撤）|1. 回测资源弹性伸缩（基于队列自动扩缩容）；<br/>2. 回测结果校验（与实盘数据对比）；<br/>3. 回测参数版本管理，支持回测结果复现；<br/>4. 多租户回测资源隔离，防止资源抢占；<br/>5. 回测报告合规化（包含风险提示、数据来源说明）|
|**策略管理服务**|策略代码管理、编译、测试|集成 Git 版本控制，自动化编译插件，策略安全审计|1. 策略代码安全扫描（防恶意代码/违规调用）；<br/>2. 策略编译沙箱，防止编译过程影响系统；<br/>3. 策略版本灰度发布，支持灰度放量；<br/>4. 策略研发权限控制（读/写/编译/发布分级）；<br/>5. 策略代码审计日志，满足合规要求|
|**盘后审计服务**|盘后风险分析与合规审计|生成每日/每月风控报告，检查是否符合监管要求，异常交易行为分析|1. 监管合规报表自动生成（如反洗钱、异常交易报告）；<br/>2. 盘后批量复盘当日所有订单、成交、持仓；<br/>3. 统计超限行为、异常交易记录；<br/>4. 违规行为日志归档、审计追溯；<br/>5. 行情数据源切换合规性校验（是否符合容灾策略）|
|**服务注册发现服务**|微服务注册、发现、健康检查|基于 etcd 实现，支持服务健康状态实时检测|1. 服务熔断降级，防止雪崩；<br/>2. 服务负载均衡（按实例性能/负载）；<br/>3. 服务下线灰度，支持优雅退出；<br/>4. 跨机房服务发现，优先调用同机房实例|
|**容灾备份服务**|核心数据备份、跨机房同步、故障恢复|基于增量备份 \+ 实时同步，支持一键恢复|1. 核心交易层数据实时同步至备机房；<br/>2. 定时全量备份（每日）\+ 增量备份（分钟级）；<br/>3. 备机房热备，故障自动切换；<br/>4. 恢复流程自动化，支持 RTO/RPO 达标验证；<br/>5. 各 **engine_id** 分片配置与行情源配置同步备份，故障恢复后自动恢复|

### 核心约束（强制）

1. 所有支撑服务**不得与交易引擎部署在同一物理机/虚拟机**

2. 支撑服务**禁止主动调用交易引擎**（不向引擎发起 gRPC/HTTP）；配置增量由引擎 **config_client 出站订阅** `WatchConfig` 接收

3. 交易引擎向支撑服务的旁路上报**经 `client/` 异步接口单向投递**，不等待响应；各服务接收端内部实现架构不约束

4. **引擎业务配置**与**实例品种归属**变更**仅经 config-service 校验后**由 `WatchConfig` 流推送；**交易凭证**变更经 account-service，**禁止**写入 `EngineConfig` 或随 Watch 推送

5. **配置拉取**：冷启动 `GetConfig` 拉全量 `EngineConfig`（gRPC，超时独立、失败则使用本地兜底快照）；运行时 `WatchConfig` 收增量；凭证 `ResolveCredential` 单独按需拉取；均不在 A 段同步执行

---

## 五、接入层（外部独立项目，企业级安全与多租户）

> **仓库边界**：API 网关、控制台前端、多租户运营界面等 **接入层实现位于 QTrade 体系外的独立项目/仓库**（或机构统一 API 平台），**不在 `qtrade` 本仓库**。本仓库交付 **交易引擎 + 支撑微服务（gRPC）**；接入层负责将外部 **HTTP/HTTPS + REST** 转为对内 **gRPC** 调用。

接入层**不提供直连交易引擎的能力**；仅将外部请求路由至 QTrade **支撑服务**：**数据查询**、**监控查看**、**配置提交（→ config-service）**、**账户与凭证管理（→ account-service）**、**回测提交**、**审计报表**、**策略生命周期配置（启停/参数，经配置中心异步生效）**。

**明确禁止**：接入层下单、直连修改 OMS/持仓、绕过 config-service 写引擎内存；禁止外部系统直连 `qtrade_engine` 的任何 TCP/HTTP/gRPC 端口。

|组件名称|功能说明|部署方式|企业级优化点|
|---|---|---|---|
|**前端控制台**|查询与配置：订单/持仓/绩效查看、监控大盘、策略参数与生命周期配置提交、**实例品种归属**配置|**外部独立项目**；经 API 网关访问|1. 多租户视图隔离；<br />2. 按钮级权限（查询 vs 配置提交 vs 审计只读）；<br/>3. 操作日志全记录；<br/>4. 敏感数据脱敏；<br/>5. 多角色（管理员/交易员/审计员）；<br/>6. **不提供「一键下单」类交易操作**|
|**API 网关**|统一北向入口：认证、限流、REST ↔ gRPC 路由|**外部独立项目**或机构统一 API 平台；与 `qtrade` 进程分离部署|1. 统一身份认证（OAuth2.0/SAML/ 企业 SSO，可对接 Keycloak）；<br />2. 租户级限流（QPS/并发数）；<br/>3. API 调用审计日志；<br/>4. API 版本管理与 OpenAPI 契约；<br/>5. 安全防护（防重放/CORS/WAF 等）|
|**第三方系统适配器**|对接资管、清算等外部系统|**外部独立项目**或网关侧插件，按需扩展|1. 接入认证；<br />2. TLS 1.3；<br/>3. 对接日志与对账；<br/>4. 故障隔离；<br/>5. 协议标准化|
|**多租户管理组件**|租户生命周期、资源配额、权限与 **engine_id** 映射|**外部独立项目**，与网关、config-service 协同|1. 租户创建/暂停/注销；<br />2. 资源配额；<br/>3. 租户级数据隔离；<br/>4. 角色模板；<br/>5. 租户 ↔ 分片配置管理（§2.4）|

#### 5.1 与本仓库的集成边界

| 方向 | 协议 | 说明 |
|---|---|---|
| 外部 → 接入层 | HTTP/HTTPS + REST | OpenAPI 由**接入层项目**维护，非本仓库 |
| 接入层 → QTrade 支撑服务 | gRPC + Protobuf | 调用 `config-service`、`account-service`、`history-service`、`observability-service` 等（本仓库 `src/service/`） |
| 接入层 → 交易引擎 | **禁止** | 配置变更仅 **config-service → 引擎出站 `WatchConfig`** |
| QTrade 支撑服务 → 引擎 | **禁止主动 RPC 引擎** | 与 §4、§8.1 一致 |

北向网关可选用 **grpc-gateway/自研 BFF/Kong+transcoding** 等实现，属于接入层项目选型；本仓库仅需保证 **支撑服务 gRPC 接口稳定、可版本化**。

### 核心约束（强制）

1. 接入层**不提供任何直连交易引擎的 API**

2. 所有配置与策略生命周期变更必须经 **config-service → gRPC WatchConfig** 异步下发（引擎出站订阅）

3. 前端**禁止**下单、撤单、手动改单等交易操作入口；策略启停/参数变更属于**控制面配置提交**，非数据面干预

4. **实例品种归属**与行情源配置经 **外部接入层 → config-service** 提交（按 `engine_id` 下发）；禁止绕过 config-service

---

## 六、企业级治理层

|组件名称|功能说明|技术实现|核心价值|
|---|---|---|---|
|**统一认证授权中心**|全系统身份认证、权限管理、令牌管理|基于 Keycloak 实现，支持 RBAC/ABAC 权限模型|1. 单点登录（SSO），统一身份管理；<br />2. 细粒度权限控制，满足最小权限原则；<br/>3. 令牌生命周期管理，自动过期/吊销；<br/>4. 支持多因子认证（MFA），强化安全；<br/>5. 实例分片与行情源配置操作权限细粒度管控|
|**数据安全组件**|数据加密、脱敏、访问控制|透明加密存储，动态脱敏，数据访问审计|1. 敏感数据（账户/资金/交易记录）加密存储；<br />2. 动态脱敏（按角色/场景展示不同粒度）；<br/>3. 数据访问水印，防止数据泄露；<br/>4. 满足等保三级/金融数据安全要求|
|**自动化运维平台**|部署、监控、告警、回滚、扩缩容|Kubernetes/裸机 \+ CI/CD|1. **生产推荐裸机或专用 VM**（isolcpus、hugepages、NUMA）；容器化需 Guaranteed QoS + CPU 绑核；<br />2. 微服务自动扩缩容；<br/>3. 发布流水线（构建/测试/部署/验证）；<br/>4. 故障自愈与运维审计|
|**合规管理平台**|监管规则配置、合规检查、报表生成、监管报送|规则引擎 \+ 报表引擎|1. 监管规则动态配置，无需修改代码；<br />2. 实时合规检查，事前预防违规；<br/>3. 自动生成监管报送文件，对接监管系统；<br/>4. 合规整改跟踪，闭环管理；<br/>5. 行情数据源切换合规审计，满足监管留痕要求|

---

## 七、适配层设计详细说明

### 7.0 双向适配器模式（Api + Spi）

券商/行情 SDK（如 EMT）是**双向 API**：引擎主动调用柜台（Api），柜台异步回调引擎（Spi）。适配层对每一侧各实现一个适配器，共同完成 **Adapter（适配器）模式**，但继承方向相反。

#### 7.0.1 角色定义

| 概念 | 行情 | 交易 | 说明 |
|------|------|------|------|
| **Target（稳定接口）** | `qtrade_sdk::quote::QuoteApi`/`QuoteSpi` | `qtrade_sdk::trader::TraderApi`/`TraderSpi` | 引擎与插件契约，定义于 `include/qtrade_sdk/` |
| **Adaptee（厂商接口）** | `EMT::API::QuoteApi`/`QuoteSpi` | `EMT::API::TraderApi`/`TraderSpi` | 外部 SDK，结构体与回调签名由厂商定义 |
| **Api 适配器** | `EmtQuoteApi` | `EmtTraderApi` | **public 继承 Target Api**，内部持有 Adaptee Api 并转发 |
| **Spi 适配器** | `EmtQuoteSpi` | `EmtTraderSpi` | **public 继承 Adaptee Spi**（接入 SDK 后），内部持有 Target Spi 指针并转发 |
| **引擎侧接入** | `QuoteNormalizer` 持有 `QuoteApi` | `TraderNormalizer` 接回报回调 | EMS 持出站 Api；回报见 §3.1 |

#### 7.0.2 调用方向（主动 vs 被动）

```text
主动侧（Api）：
  引擎 ──调用──► XxxApi 适配器 : qtrade_sdk::XxxApi ──转发──► 厂商 XxxApi

被动侧（Spi）：
  厂商 XxxSpi ──回调──► XxxSpi 适配器 : 厂商::XxxSpi ──转换+委托──► 引擎实现的 qtrade_sdk::XxxSpi
```

**要点**：Spi 适配器**不**继承 `qtrade_sdk::*Spi`。`qtrade_sdk::*Spi` 是引擎要实现的 Target；Spi 适配器继承的是厂商回调接口，在 override 中做结构体转换后调用 `target_->On*()`。

#### 7.0.3 接线示例（以 EMT 行情为例）

```cpp
// Api：适配器即 Target 的实现
class EmtQuoteApi final : public qtrade_sdk::quote::QuoteApi {
  void RegisterSpi(qtrade_sdk::quote::QuoteSpi& spi) override {
    emt_spi_.SetTarget(&spi);
    // emt_api_->RegisterSpi(&emt_spi_);
  }
  // Subscribe 等 → 转发至 emt_api_
 private:
  EmtQuoteSpi emt_spi_;
  // EMT::API::QuoteApi* emt_api_;
};

// Spi：适配器即厂商回调的接收者
class EmtQuoteSpi final : public EMT::API::QuoteSpi {
  void OnDepthMarketData(EMT::EMTMarketData* data, ...) override {
    // EMT 结构体 → qtrade_sdk::MarketTick
    target_->OnDepthMarketData(tick, bid1_qty, ask1_qty);
  }
 private:
  qtrade_sdk::quote::QuoteSpi* target_ = nullptr;
};
```

#### 7.0.4 代码目录约定

```text
src/adapter/
├── mock/quote|trader/     # 无厂商依赖：MockXxxApi : Target Api；MockXxxSpi 模拟厂商回调
└── emt/quote|trader/      # EMT 生产：EmtXxxApi + EmtXxxSpi 成对出现
```

工厂函数：`qtrade::adapter::mock::*::CreateMock*Api()` 返回 Target Api 指针，仅 demo/测试链接 mock 适配器时使用。

#### 7.0.5 与引擎内部分发的关系

适配器 → Normalizer → EventBus → Handler，路径见 **§3.1、§8.2**。Normalizer 仅语义统一 + `Publish*`（与 `QuoteNormalizer` 对称）。

### 7.1 行情数据源（QuoteApi + QuoteNormalizer）

#### 7.1.1 设计原则：配置驱动分片，与代码实现对齐

MVP 也**不单独引入 `QuoteSourceManager` 类**（代码中不存在该模块）。分片由 **§2.4 配置驱动多实例** 在部署层完成；进程内行情链路为：

```text
行情适配器（QuoteApi 实现，如 MockQuoteApi/EmtQuoteApi）
  → QuoteNormalizer（SetQuoteApi/Subscribe/语义标准化/OnTick → Lane-M）
```

- **`qtrade_sdk::quote::QuoteApi`**：稳定 Target 接口；各厂商 **Api 适配器**实现（§7.0）
- **`QuoteNormalizer`**：引擎侧入口；持有 `std::unique_ptr<QuoteApi> market_source_`；注册 `SetTickCallback` 或 `RegisterSpi`；语义标准化后向 Lane-M 发布 Tick/Bar
- **instruments 过滤**：`QuoteNormalizer::Subscribe` 传入本实例各策略绑定品种的**并集**（去重，且并集内每品种唯一，§2.4.3）；Lane-M 按品种 **1:1** 投递至绑定策略
- **failover（规划）**：配置中 `quote_source`/`quote_failover` 由引擎在断线时**重建或切换 `QuoteApi` 实例**（或重连同一适配器），逻辑可放在 `QuoteNormalizer` 或启动编排层

#### 7.1.2 抽象接口层（Target）

稳定接口定义于 `qtrade_sdk/quote/`（`QuoteApi`、`QuoteSpi`），引擎与适配器仅依赖此层。核心能力包括：

- 连接管理：`Connect`/`Disconnect`/`IsConnected`

- 订阅控制：`Subscribe`/`Unsubscribe`（及厂商原生订阅接口）

- 数据回调：`QuoteSpi::OnDepthMarketData` 等（由 Spi 适配器转发触发）

- 便捷回调：`SetTickCallback`/`SetBarCallback`（引擎内部路径，可与 Spi 并存）

- 健康与错误（MVP 推荐）：`GetHealthStatus()`、`GetErrorCode()`

- 企业级扩展（二期+）：`GetLoadMetrics()`、预连接双源切换指标等

> 历史文档中的 `QuoteApi` 已统一为 `qtrade_sdk::quote::QuoteApi`（别名 `QuoteApi`）。详见 §7.0。

#### 7.1.3 行情源切换策略（分阶段）

| 策略 | 阶段 | 核心逻辑 | 说明 |
|---|---|---|---|
| **失败后再切换（默认）** | MVP | 单连接；断线/超时 → 重连或切 `quote_failover` 地址 | 实现简单；切换窗口可能丢 Tick，需在 SLA 中明示 |
| **本地环形缓冲** | MVP | 短时断流兜底 | 配合失败切换，减少策略误判 |
| **预连接主备双源** | 可选/二期 | 主备同时 Subscribe，本地选 active + 去重 | 切换目标 < 5ms、少 gap；成本为双带宽与去重逻辑 |
| **Tick/Bar 分流** | 二期 | Lane-M 内 Tick 高优先级队列，Bar 低优先级 | 可绑独占核 |
| **维护窗口调片** | 二期+ | 修改 config 中策略/品种归属，重启实例 | 水平扩容：加实例 + 改配置，而非进程内自动迁移 |
| **跨机房接入点选择** | 三期 | 按 RTT 选择行情接入点 | 与 §9.1 Active-Passive 协同 |

**多核绑定（可选）**：单实例内 Tick 量极大时，Lane-M 消费线程 `sched_setaffinity` 绑核；**不**等同于跨进程分片。

#### 7.1.4 插件实现层

- 每种数据源（如 EMT、CTP、Wind）在 `src/adapter/<vendor>/quote/` 提供 **Api + Spi 成对实现**（§7.0），`MockQuoteApi`/`MockQuoteSpi` 用于开发测试
- 编译为独立动态库（.so/.dll），包含插件元数据（支持的品种、数据类型、版本、兼容系统版本）
- Spi 适配器负责厂商结构体 → `qtrade_sdk` 类型的转换；Api 适配器负责引擎请求 → 厂商 API 的转发
- 企业级扩展：
  - 插件签名校验，防止恶意插件；
  - 插件版本兼容检测，避免版本冲突；
  - 插件运行状态监控，上报资源占用/错误率


#### 7.1.5 引擎侧组件（与 `src/engine/normalizer/` 一致）

- **适配器工厂**：如 `CreateMockQuoteApi()`（`src/adapter/mock/quote/`），返回 `std::unique_ptr<QuoteApi>`
- **QuoteNormalizer**：`SetQuoteApi(std::move(api))` 注入适配器；`Subscribe`/`Unsubscribe` 转发至 `QuoteApi`；语义标准化后 `OnTick`/`OnBar` → Lane-M
- **failover（二期）**：断线检测后按配置切换 `quote_failover` 对应适配器或重连；切换事件 P0 审计
- **负载监控（二期+）**：实例级指标旁路上报 observability-service

企业级扩展：插件签名校验、灰度加载、故障降级、切换操作审计、控制面热更新行情源地址（**不**热更新跨实例品种归属，归属变更走维护窗口）。

### 7.2 交易执行模块适配器

- **抽象接口层（Target）**：定义于 `qtrade_sdk/trader/`（`TraderApi`、`TraderSpi`），包含：

    - 连接管理：`Connect`/`Disconnect`/`Login`/`Logout`

    - 订单操作：`SendOrder`/`InsertOrder`/`CancelOrder`

    - 查询功能：`QueryOrders`/`QueryTrades`/`QueryPositions`/`QueryAsset`

    - 回报回调：`TraderSpi::OnOrderEvent`/`OnTradeEvent` 等（由 Spi 适配器转发触发）

    - 企业级扩展：`GetConnectionHealth()`（连接健康度）、`GetFlowControlStatus()`（流量控制状态）、`SetFailoverConfig()`（故障转移配置）、`GetExecutionCost()`（执行成本统计）

- **适配器实现层**（§7.0 双向适配）：

    - 每个经纪商在 `src/adapter/<vendor>/trader/` 提供 `XxxTraderApi` + `XxxTraderSpi`
    - **Api 适配器**继承 `TraderApi`，将发单/撤单/查询转发至厂商 API
    - **Spi 适配器**继承厂商 `TraderSpi`，将回报解析为 `qtrade_sdk` 类型后调用引擎注册的 `TraderSpi`
    - **引擎侧**：回报 `TraderNormalizer` → Lane-R → `ReturnHandler` → OMS；出站 EMS → `TraderApi`
    - 企业级扩展：
      - 适配器协议版本兼容，支持交易所协议升级；
      - 适配器流量控制，适配交易所限流规则；
      - 适配器故障重试策略（指数退避）；
      - 适配器交易成本统计，支持最优路由选择
- **管理组件**：

    - 执行适配器管理器：加载/切换适配器插件，监控连接状态
    - 订单路由：根据品种、流动性、成本选择最优执行适配器
    - 故障恢复：适配器异常时自动切换备用插件，重发未完成订单
    - 企业级扩展：
      - 多路径订单路由（主备通道自动切换）；
      - 适配器容灾管理器（跨机房适配器自动切换）；
      - 订单执行审计（发单/撤单/成交全记录）； 
      - 适配器性能监控（延迟/成功率/失败率）；
      - 订单幂等性保障，防止重复执行

### 7.3 策略插件（Strategy Plugin）

- **抽象基类层**：定义`IStrategy`基类，包含：

    - 生命周期方法：`Init(config)`、`Start()`、`Pause()`、`Resume()`、`Stop()`

    - 事件处理方法：`OnTick(tick)`、`OnBar(bar)`、`OnOrder(order)`、`OnTrade(trade)`

    - 交互方法：`SendSignal(signal)`、`GetParameter(key)`、`SetParameter(key, value)`

    - 企业级扩展：`GetResourceUsage()`（资源占用上报）、`SetResourceLimit()`（资源限制）、`OnRiskAlert()`（风控告警回调）、`GetVersion()`（策略版本）
- **策略实现层**：

    - 具体策略继承 `IStrategy`，编译为 `.so/.dll` 插件
    - **故障隔离**（同进程）：`try/catch` 边界 + 策略级熔断 + 资源配额；单策略未捕获异常时暂停该策略实例，**不承诺**防内存破坏级恶意代码
    - **安全隔离**（发布前）：策略管理服务内静态扫描 + 签名校验 + 沙箱编译；生产插件仅加载已审计版本
    - 策略仅接收本实例 Lane-M/Lane-R 事件；Tick/Bar 按 **instrument → strategy** **1:1** 分发（§2.4.3）；同品种多策略须部署在不同 `engine_id`
- **管理组件**：

    - 策略工厂/调度器/仓库：加载、并行调度、版本与灰度
    - 生命周期变更（Start/Pause/Stop）由**控制面**写入配置，引擎控制线程应用，**不产生订单**

### 可插拔核心优势

1. 新增行情源、交易通道、策略无需修改核心代码

2. 插件独立编译、独立部署、独立升级

3. 插件故障隔离，不影响核心引擎运行

4. 支持插件灰度发布与回滚

---

## 八、模块与微服务交互方式（企业级可靠性增强）

### 8.1 交互方式说明

- **交易引擎内部**：Lane-M/Lane-R 独立无锁队列（§3.1）；`QuoteApi` 适配器与 `QuoteNormalizer` 间为内存回调（`SetTickCallback`/Spi）

- **交易引擎 → 支撑服务（D 段）**：经 `log_client`、`monitor_client` 等**异步上报接口**单向投递（A 段仅 `enqueue`；**Outbound 线程**调用 client）；载荷 Protobuf；**不等待响应**；client 内部传输（gRPC/HTTP/本地 Spool/no-op）**架构不规定**，MVP 允许 stub

- **引擎 ↔ config-service（控制面，全程 gRPC）**：
  - 冷启动：`GetConfig` 一次性拉全量 `EngineConfig`（独立超时）；失败则读本地兜底快照
  - 运行时：引擎以 **gRPC Client 出站**调用 `WatchConfig`（Server Streaming），推送 `ConfigSnapshot`（`version` + `engine`）；**独立控制线程**消费并更新本地快照，**不阻塞 A 段**
  - **禁止**：引擎对外提供 gRPC Server；禁止 config-service 主动 RPC 调用引擎

- **引擎 ↔ account-service（凭证面，启动/换密阶段 gRPC）**：
  - 冷启动 / 换密：`ResolveCredential(engine_id, account_id)` 按需拉取登录材料；结果**仅驻留进程内存**，供 EMS `Connect`/`Login`
  - **禁止**：密码/token 进入 `EngineConfig` 或 `WatchConfig` 流；禁止策略插件直接调用 account-service

- **支撑服务之间**：**gRPC + Protobuf**（如 config 写入前向 account-service 校验账户授权）

- **外部 → 交易引擎**：**禁止**；外部经接入层 → 支撑服务

#### 控制面 gRPC 接口（概念）

| RPC | 类型 | 调用方 | 用途 |
|---|---|---|---|
| `GetConfig` | Unary | engine → config-service | 冷启动全量 `EngineConfig` |
| `WatchConfig` | Server Streaming | engine → config-service | 运行时配置变更（`ConfigSnapshot`） |
| `ResolveCredential` | Unary | engine → account-service | 冷启动/换密时解析登录凭证 |
| `RegisterAccount` / `RotateCredential` | Unary | 接入层 → account-service | 账户与凭证管理（运维） |

断线时控制线程指数退避重连；重连期间沿用本地快照与已登录 session，交易不中断（换密场景需 EMS 重连策略，二期）。

#### 旁路背压策略

| 数据级别 | 示例 | 远程上报不可用时的行为 |
|---|---|---|
| **P0 审计** | 合规拦截、发单/撤单流水 | 本地 spool 环形缓冲（有界）→ 恢复后重放；缓冲满则**拒写新 P0 并告警**（不阻塞 A 段） |
| **P1 业务** | 订单状态、成交副本 | 有界缓冲，满则丢弃最旧批次并计数告警 |
| **P2 指标** | 延迟直方图、队列深度 | 可丢弃 |
| **P3 调试** | verbose 日志 | 可丢弃 |

原则：**A 段永不因旁路满而阻塞**；P0 通过本地 spool + 运维告警保障最终一致；长期归档由 observability/audit 服务写入时序库或对象存储（≥3 年），不由引擎 client 保证。

### 8.2 核心数据流（最关键）

引擎内数据面统一为 **适配器 → Normalizer → EventBus → Handler**（双 Reactor 见 §3.1）。

| 通道 | 适配器 → Normalizer → `Publish*` | 出队后 Handler |
|---|---|---|
| **Lane-M** | 行情适配器 → `QuoteNormalizer` | `StrategyEngine` → CMS → Risk → OMS → EMS |
| **Lane-R**（优先） | 交易适配器 → `TraderNormalizer` | `ReturnHandler` → OMS（同步 Account/PMS）→ `StrategyEngine` |

**1. 行情 → 发单（A + C；B 并行）**

```text
外部行情 → 适配器 → QuoteNormalizer → Lane-M
  → StrategyEngine → CMS → Risk → OMS → EMS 入队              [A 段]
  → EMS 出队 → 适配器 → 交易所                                [C 段，§1.3.4]
  ╲ OMS 变更 ──► B 段 WAL / 合规日志（异步，不挡 A/C）
```

**2. 订单/成交回报（Lane-R）**

```text
交易所 → 适配器 → TraderNormalizer → Lane-R
  → ReturnHandler → OMS（同步 Account/PMS）→ StrategyEngine
  ╲ OMS 变更 ──► B 段 WAL（异步，可选）
```

**3. 控制面**

```text
用户 → API 网关 → config-service / account-service
  ├─ 冷启动: engine ──GetConfig(engine_id)──► EngineConfig（策略、account_id 引用）
  ├─ 冷启动: engine ──ResolveCredential──► 登录材料（不进 Watch 流）
  └─ 运行时: engine ──WatchConfig（出站）──► 控制线程 → 本地 EngineConfig 快照
```

**4. D 段旁路 / 外部 API**

```text
A 段 enqueue → Outbound → log_client / monitor_client / …（fire-and-forget）
外部 → 接入层（HTTP）→ 支撑服务 gRPC  ✗ 不直连 qtrade_engine
```

### 8.3 协议规范

|交互场景|推荐协议|核心理由|
|---|---|---|
|交易引擎内部|内存结构体 + Lane-M/Lane-R 无锁队列|A 段微秒级；回报优先于行情（§3.1）|
|交易引擎 ↔ 适配器|函数调用 + 回调|同进程插件，无网络|
|交易引擎 → 支撑服务（D 段旁路）|`client/` 异步接口 + Protobuf|A 段仅入队；Outbound 线程 fire-and-forget；内部传输可插拔|
|引擎 ↔ config-service（控制面）|gRPC + Protobuf|`GetConfig` / `WatchConfig` 下发 `EngineConfig`；引擎仅作 Client|
|引擎 ↔ account-service（凭证面）|gRPC + Protobuf|`ResolveCredential` 按需拉取；不进 Watch 流；启动/控制线程执行|
|支撑服务之间|gRPC + Protobuf|强类型 RPC，适合查询与批量数据（如回测拉历史）|
|接入层 ↔ 外部|HTTP/HTTPS + REST|**外部接入层项目**维护 OpenAPI；REST ↔ gRPC 转换在接入层完成|
|外部接入层 → QTrade 支撑服务|gRPC + Protobuf|本仓库 `src/service/` 暴露 gRPC；不对外 HTTP|

---

## 九、企业级容灾与数据一致性设计

### 9.1 部署架构与分片模型

#### 分片键与主备模式

- **分片键**：`tenant_id + account_id + instrument_id`（写入各实例配置，见 §2.4）
- **水平扩展**：部署多个引擎实例，每实例单进程全封闭；实例间**不共享**内存状态
- **每分片 Active-Passive（二期）**：同一配置（相同 `engine_id` 分片组）下 Standby 实例跟行情/WAL，**不对外发单**；Active 故障时升主（RTO < 30s）

#### 部署拓扑（分阶段）

| 阶段 | 拓扑 | 说明 |
|---|---|---|
| MVP | 单机房、**N 实例 + 静态配置分片** | 每实例 JSON/config 指定 instruments；无跨机房；通常无 Standby |
| 二期 | 单机房多实例 + 分片主备 | 配置驱动分片 + Active/Standby 实例对；维护窗口改配置调片 |
| 三期 | 双机房 Active-Passive + 灾备 | 主机房 Active 集合；备机房热 Standby；灾备冷备 |

- **支撑服务层**：二期起多副本；ClickHouse 三副本与跨机房同步放在二期/三期
- **Lite 档位（可选）**：单租户/资源受限时，允许 config-service、observability 与 engine **同 VM** 部署，但 engine 须 CPU 绑核，且**不引入独立消息中间件**
- **网络隔离**：交易引擎独立内网，与支撑服务、接入层防火墙隔离
- **行情**：每实例 `QuoteNormalizer` + `QuoteApi` 适配器；failover 与跨机房接入点选择分阶段增强（§7.1.3）

### 9.2 故障切换策略

- **进程级故障**：自动重启 + 本地 WAL replay（RTO < 10s）；控制面快照本地恢复；replay 时长与 WAL 体积纳入压测

- **节点级故障（分片内）**：Standby 升 Active；未完成订单按 **order_id 幂等** 与交易所对账后决定是否重发；**跨实例品种迁移**仅维护窗口改 config（§2.4）

- **机房级故障**：备机房 Standby 集合升 Active（RPO < 1min，RTO < 5min）；切换后**强制对账** + 可选熔断直至一致

- **行情源故障**：MVP 默认失败后再切换 + 本地缓冲；选用预连接双源时切换目标 < 5ms；切换事件 P0 审计

### 9.3 数据一致性保障

- **核心交易数据**：采用 WAL（预写日志）\+ 内存快照机制，确保进程重启后数据不丢失；

- **跨机房数据同步**：采用 “实时增量同步 \+ 定时全量校验”，确保数据一致性；

- **订单幂等性**：通过全局唯一订单 ID 确保发单/撤单操作幂等，防止重复执行；

- **对账机制**：与交易所/清算**定时对账**（频率可配置，默认日终 + 故障切换后强制）；不一致时告警 + 熔断该账户分片直至人工确认

- **行情一致性**：主备源切换时序号/时间去重；跨实例品种归属变更仅在维护窗口，变更前 WAL 刷盘 checkpoint

---

## 十、企业级安全与合规设计

### 10.1 安全防护

- **网络安全**：核心交易层部署在私有网络，接入层通过防火墙/入侵检测系统（IDS）防护；API 网关支持 DDoS 防护、IP 白名单；

- **数据安全**：敏感数据（账户凭证/资金/策略参数）加密存储（AES\-256），传输加密（TLS 1.3）；**交易密码仅存 account-service 密文表**，不得进入 `EngineConfig`；数据访问需多因子认证

- **代码安全**：策略/插件代码安全扫描，防止恶意代码；核心代码定期安全审计

- **操作安全**：关键操作（如策略发布、资金划转、**实例品种归属**修改）需双人复核；操作日志不可篡改，支持审计溯源

### 10.2 合规适配

- **金融监管**：满足《证券期货经营机构私募资产管理业务管理办法》《期货交易管理条例》等监管要求，支持交易留痕、反洗钱、异常交易监控；行情数据源切换记录纳入合规审计，满足监管留痕要求

- **数据合规**：满足等保三级、数据安全法、个人信息保护法要求，支持数据脱敏、跨境数据合规；

- **审计合规**：审计日志留存≥3 年，支持司法取证；定期生成合规报告，对接监管报送系统；行情源切换与实例配置变更日志独立归档，支持监管溯源

---

## 十一、核心风险点与架构应对措施

|风险点|架构层面应对措施|分片/行情相关|
|---|---|---|
|行情源中断|换/重连 `QuoteApi` + 本地缓存（规划）|MVP 失败后再切；可选预连接双源；按实例隔离|
|行情处理过载|配置驱动多实例水平扩展 + 可选 CPU 绑核|二期：Lane-M 内 Tick/Bar 分流；维护窗口改配置调片|
|交易通道故障|多通道切换 + 幂等重发|—|
|策略异常|异常捕获 + 策略熔断 + 资源配额；发布前扫描|故障限定于单实例配置内品种 |
|交易引擎崩溃|WAL + 快照 + 自动重启|控制面本地快照恢复|
|配置更新异常|灰度 + 回滚 + 本地兜底|品种归属变更需维护窗口|
|数据不一致|幂等 order_id + 对账 + 切换后熔断|主备行情去重|
|人为干预|数据面封闭 + 控制面经 config 审计|关键配置双人复核|
|回报被行情阻塞|Lane-R 优先于 Lane-M（§3.1）|Account/PMS 走 OMS 同步路径|

---

## 十二、架构演进路线图

|阶段|建设重点|交付目标|分片/行情|
|---|---|---|---|
|**一期（MVP）**|核心引擎、基础适配器、config + **account** + observability|单租户实盘（dry-run → 实盘）|**配置驱动多实例** + 账户凭证分服务 + QuoteApi/QuoteNormalizer|
|**二期**|多租户、策略管理、审计、容灾、服务拆分|多机构并行|实例 Active-Passive + Lane-M 内 Tick/Bar 分流/维护窗口改配置|
|**三期**|跨机房主备、监管报送、智能风控|金融生产级|跨机房行情接入点选择|

### 12.1 文档能力 ↔ 代码里程碑（当前仓库）

| 架构能力 | 目标阶段 | 代码现状（参考 `src/`） |
|---|---|---|
| EventBus + 引擎骨架 | MVP | ✅ 已有 |
| EventLanes（双 EventReactor EventBus） | MVP | ✅ EventReactorLoop + event_types |
| CMS/OMS/EMS/风控/持仓 | MVP | 🟡 模块骨架，WAL/幂等待完善 |
| 配置驱动分片（`EngineConfig` + engine_id + 一品种一策略） | MVP | 🟡 proto/DB 已对齐 `EngineConfig`；config 校验与 StrategyEngine 1:1 分发待实现 |
| 交易账户服务（account-service） | MVP | ❌ 文档已定义（§2.5）；代码待落地 |
| 凭证与配置分离（密码不进 WatchConfig） | MVP | 🟡 架构约束已写入；account-service 待实现 |
| QuoteNormalizer + QuoteApi | MVP | ✅ 已有（`SetQuoteApi`、Subscribe、PublishTick）；failover 未实现 |
| TraderNormalizer + TraderApi 回报 | MVP | 🟡 骨架已有；语义标准化与 OMS/适配器串联待完善 |
| 旁路 client（log/monitor） | MVP | 🟡 接口已有，远程上报未实现，引擎未集成 |
| config_client gRPC（GetConfig + WatchConfig） | MVP | 🟡 config_client 待接入引擎 |
| 支撑微服务 | MVP | 🟡 多为 stub |
| 接入层（外部独立项目） | 二期（非本仓库） | — 网关/控制台由外部项目实现；本仓库提供稳定 gRPC 供其调用 |
| 分片 Active-Passive | 二期 | ❌ |
| 跨机房容灾 | 三期 | ❌ |

图例：✅ 可用　🟡 进行中　❌ 未开始

---
