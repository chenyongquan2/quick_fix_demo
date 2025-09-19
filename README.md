# quick_fix_demo

Echo FIX 4.4 server (acceptor) and client (initiator) using QuickFIX C++, built with xmake and dependencies managed via Conan.

## Prerequisites

- Git Bash or PowerShell on Windows 10+
- Python 3.8+
- Conan 2.x (`pip install conan`)
- xmake (`choco install xmake` or see `https://xmake.io`)

Initialize Conan client (first time only):

```bash
conan profile detect --force
```

Update sub module code.
```bash
git submodule update --init --recursive
```

If the git link hash relating from the sub-module vary, we should commit it in the parent repo
git commit info example:
```Text
 Update cc-common submodule to latest commit.
```

## Build

```bash
# from repo root
xmake f -c -y
xmake -y
```

This builds two binaries:
- `echo_acceptor`
- `echo_initiator`

## Run

Open two terminals.

Terminal A (server):

```bash
./build/windows/x64/release/echo_acceptor.exe config/acceptor.cfg
```

Terminal B (client):

```bash
./build/windows/x64/release/echo_initiator.exe config/initiator.cfg
```

By default, the server listens on port 5001. The client connects to `127.0.0.1:5001`. Logs and stores are under `log/` and `store/`.

## FIX 4.4 dictionary and Nelogica notes

For strict validation compatible with FIX 4.4 (20030618 errata), set in `config/*.cfg`:

```ini
UseDataDictionary=Y
DataDictionary=spec/FIX44.xml
```

Provide a suitable `spec/FIX44.xml` (QuickFIX ships a default). For Nelogica gateway specifics (subset, custom tags), extend the dictionary or disable strict checks during early development.

## Project structure

```
config/            # session configs for acceptor & initiator
src/               # C++ sources
log/, store/       # runtime files
spec/              # place FIX44.xml here if using dictionary
xmake.lua          # build file
```

## Configuration reference (EN)

The configs live under `config/acceptor.cfg` and `config/initiator.cfg` using INI-style sections.

### Sections
- **[DEFAULT]**: Defaults applied to all sessions unless overridden.
- **[SESSION]**: One FIX session definition (version, CompIDs, heartbeats).

### Common keys
- **ConnectionType**: `acceptor` (server) or `initiator` (client).
- **SocketAcceptPort / SocketConnectHost / SocketConnectPort**: Listen port for acceptor; remote host/port for initiator.
- **StartTime / EndTime**: Session active window (local time). Outside the window sessions won’t connect or will log out.
- **FileStorePath**: Persistent store for sequence numbers and message bodies.
- **FileLogPath**: Event and message logs directory.
- **UseDataDictionary**: `Y` enables FIX dictionary validation; `N` disables.
- **DataDictionary**: Path to XML spec (e.g., `spec/FIX44.xml`) when validation is on.
- **ResetOnLogon**: `Y` resets sequence numbers to 1 on logon; `N` preserves history.
- **ValidateFieldsOutOfOrder**: `Y` enforces field order per dictionary; `N` is lenient.
- **UseLocalTime**: Use local clock for session window/time calculations.
- **BeginString**: FIX version (e.g., `FIX.4.4`).
- **SenderCompID / TargetCompID**: Local/remote CompID; must mirror each other across the link.
- **HeartBtInt**: Heartbeat interval in seconds.
- (Initiator) **ReconnectInterval**: Seconds between reconnect attempts.

### Example specifics in this repo
- Acceptor listens on port `5001` with `SenderCompID=ECHO_SERVER`, `TargetCompID=ECHO_CLIENT`.
- Initiator connects to `127.0.0.1:5001` with `SenderCompID=ECHO_CLIENT`, `TargetCompID=ECHO_SERVER`.
- By default, dictionary validation is off (`UseDataDictionary=N`), and sequence numbers reset on each logon (`ResetOnLogon=Y`).

## 配置说明（中文）

配置位于 `config/acceptor.cfg` 与 `config/initiator.cfg`，采用 INI 风格分节。

### 分节
- **[DEFAULT]**：默认参数，适用于所有会话，除非在 `[SESSION]` 覆盖。
- **[SESSION]**：单个 FIX 会话的定义（版本、双方 CompID、心跳等）。

### 常用键
- **ConnectionType**：`acceptor`（服务端）或 `initiator`（客户端）。
- **SocketAcceptPort / SocketConnectHost / SocketConnectPort**：服务端监听端口；客户端的目标地址与端口。
- **StartTime / EndTime**：会话活跃时间窗（本地时间）。窗口外将不连接或会登出。
- **FileStorePath**：持久化存储（序列号、消息体）。
- **FileLogPath**：事件与消息日志目录。
- **UseDataDictionary**：`Y` 启用数据字典校验；`N` 关闭校验。
- **DataDictionary**：当启用校验时，指向 XML 规范（如 `spec/FIX44.xml`）。
- **ResetOnLogon**：`Y` 在登录时将序列号重置为 1；`N` 保留历史。
- **ValidateFieldsOutOfOrder**：`Y` 按字典要求检查字段顺序；`N` 宽松处理。
- **UseLocalTime**：使用本地时间进行时间窗/时间计算。
- **BeginString**：FIX 版本（如 `FIX.4.4`）。
- **SenderCompID / TargetCompID**：本端/对端会话标识，需与对端镜像匹配。
- **HeartBtInt**：心跳间隔（秒）。
- （仅客户端）**ReconnectInterval**：断线重连间隔（秒）。

### 本仓库示例
- 服务端监听 `5001`，`SenderCompID=ECHO_SERVER`，`TargetCompID=ECHO_CLIENT`。
- 客户端连接 `127.0.0.1:5001`，`SenderCompID=ECHO_CLIENT`，`TargetCompID=ECHO_SERVER`。
- 默认关闭字典校验（`UseDataDictionary=N`），登录时重置序列号（`ResetOnLogon=Y`）。

## Best practices: Test vs Production

### Test
- Validation: `UseDataDictionary=N` (or relaxed). `ValidateFieldsOutOfOrder=N`.
- Sequence: `ResetOnLogon=Y` for quick resets.
- Heartbeats: 10–30s to surface issues quickly.
- Reconnect: `ReconnectInterval=3–5s` simple fixed interval.
- Windows: `StartTime=00:00:00`, `EndTime=23:59:59` (all day).
- Logs/Store: verbose logging, easy cleanup paths under `log/` and `store/`.

### Production
- Validation: `UseDataDictionary=Y` with `DataDictionary=spec/FIX44.xml`; `ValidateFieldsOutOfOrder=Y`. Consider enabling unknown-field/type rejection and latency checks if available.
- Sequence: `ResetOnLogon=N`; rely on persistent stores; plan coordinated resets only when necessary.
- Heartbeats: 30–60s; ensure both sides match; configure logon/logout timeouts if supported.
- Reconnect: Prefer backoff (exponential/step) or longer interval (e.g., 20–60s); avoid thrashing.
- Windows: Set to business trading hours; account for holidays.
- Logs/Store: durable volumes, retention and rotation; centralize and monitor; mask sensitive fields if required.
- Security/Network: private links/VPN; TLS where possible; IP allowlists; TCP keepalive; firewall/NAT rules defined.
- Time/Clock: NTP-synchronized hosts; enable millisecond timestamps and latency checks if available.
- Isolation: distinct CompIDs, ports, and directories per environment.

## 最佳实践：测试 vs 生产

### 测试环境
- 校验：`UseDataDictionary=N`（或宽松），`ValidateFieldsOutOfOrder=N`。
- 序列号：`ResetOnLogon=Y`，便于快速回归。
- 心跳：10–30 秒，便于暴露问题。
- 重连：`ReconnectInterval=3–5 秒`，固定间隔即可。
- 时间窗：全天 `00:00:00–23:59:59`。
- 日志/存储：详细日志，便于清理的本地目录。

### 生产环境
- 校验：`UseDataDictionary=Y` 且 `DataDictionary=spec/FIX44.xml`；`ValidateFieldsOutOfOrder=Y`。如有能力，开启未知字段/类型拒绝与时延检查。
- 序列号：`ResetOnLogon=N`；依赖持久化恢复；仅在窗口内协同重置。
- 心跳：30–60 秒；双方一致；如支持可设置登录/登出超时。
- 重连：采用退避策略或更长间隔（如 20–60 秒）；避免频繁震荡。
- 时间窗：按交易时段配置，并考虑节假日。
- 日志/存储：生产持久盘，设置保留与滚动；集中采集与告警；必要时字段脱敏。
- 安全/网络：专线或 VPN；尽量使用 TLS；白名单 IP；TCP keepalive；明确防火墙/NAT 规则。
- 时间/时钟：全节点 NTP 对时；如支持启用毫秒时间戳与延迟校验。
- 隔离：为测试/生产使用不同的 CompID、端口与目录。
