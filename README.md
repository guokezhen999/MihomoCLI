# Mihomo (Clash Meta) CLI Manager (C++ Refactored)

这是一个使用 **C++17** 重构的命令行管理工具，用于管理和控制本地 Mihomo (Clash Meta) 服务。
相比原先的 Python 脚本，C++ 版本拥有**极快的启动速度（接近 0 延迟）**、**极低的系统开销**，并且**完全不需要 Python 运行环境及三方依赖包**。

它深度集成了您原有的脚本（`start.sh`, `stop.sh`, `update.sh`）及配置文件（`preferred_node`），并提供高性能的交互式终端界面与快捷命令行指令。

---

## 目录分配与结构 (Project Directory Structure)

C++ 重构的目录分配遵循了现代 C++ 工程的最佳实践，结构划分极其合理、高内聚低耦合：

```text
clash-cli/
├── CMakeLists.txt              # CMake 构建配置文件
├── include/                    # 头文件目录
│   └── clash-cli/
│       ├── api.hpp             # REST API 客户端与 IP 查询
│       ├── config.hpp          # config.yaml 配置读取
│       ├── service.hpp         # 系统服务控制 (start/stop/logs)
│       └── tui.hpp             # 终端 TUI 功能 (Spinner/键输入捕获/搜索列表)
├── src/                        # 源文件目录
│   ├── api.cpp                 # API 请求与网络查询实现
│   ├── config.cpp              # 配置文件解析实现
│   ├── main.cpp                # 命令行入口与主菜单循环
│   ├── service.cpp             # 进程管理与脚本封装实现
│   └── tui.cpp                 # TUI 核心动画与按键交互实现
└── third_party/                # 三方头文件依赖 (完全自包含，支持离线编译)
    ├── httplib/
    │   └── httplib.h           # Header-only 高性能 HTTP 客户端
    └── nlohmann/
        └── json.hpp            # Header-only JSON 解析库
```

---

## 功能特性

1. **零延迟启动**：C++ 编译后的原生二进制执行文件，瞬间启动，无任何解释器开销。
2. **一键服务管理**：快捷启动、停止、重启代理服务。
3. **实时状态查看**：显示运行状态、进程 PID、当前路由模式、各策略组正在使用的节点等。
4. **交互式节点切换**：
   - 自动获取所有可用节点，通过多列精美对齐的菜单进行选择。
   - **支持节点搜索过滤**，无需在一长串节点中手动翻找，输入关键字即可快速筛选。
   - 自动更新您的 `preferred_node` 配置。
5. **命令行快捷切换**：
   - 支持通过 `clash-cli select <节点名称/关键字>` 快速切换节点。
   - 支持通过 `clash-cli mode <rule/global/direct>` 切换路由模式。
6. **实时日志查看**：集成 `tail -f` 日志，支持使用 `Ctrl + C` 退出。
7. **配置订阅更新**：调用原有脚本更新最新订阅并保持端口和安全规则设置。
8. **IP及归属地双模式查询**：一键同时获取本地真实公网 IP 和代理出口 IP 及其地理位置（支持 `ip-api.com` 接口及 `myip.ipip.net` 备用接口）。

---

## 编译与安装 (Build & Install)

### 1. 编译项目
请在 `clash-cli` 目录下使用以下标准 CMake 命令进行编译：

```bash
cd clash-cli
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

编译成功后，将在 `build` 目录下生成可执行程序 `clash-cli`。

### 2. 配置环境变量 / 创建软链接
为了能够在系统任何路径下直接运行 `clash-cli`，建议创建软链接到您的个人环境变量目录 `~/.local/bin` 中（该软链接已为您配置好）：

```bash
ln -sf /nfs/guokezhen/cli/clash-cli/build/clash-cli /home/skh_gkz/.local/bin/clash-cli
```

---

## 命令行指令说明

除了直接运行 `clash-cli` 进入键盘交互菜单外，您还可以直接在终端使用以下快捷子命令：

| 命令 | 说明 | 示例 |
| :--- | :--- | :--- |
| `clash-cli` | 打开键盘交互控制面板（默认） | `clash-cli` |
| `clash-cli start` | 启动代理服务 | `clash-cli start` |
| `clash-cli stop` | 停止代理服务 | `clash-cli stop` |
| `clash-cli restart` | 重启代理服务 | `clash-cli restart` |
| `clash-cli status` | 查看详细运行状态 | `clash-cli status` |
| `clash-cli mode [rule/global/direct]` | 查询或更改路由模式 | `clash-cli mode global` |
| `clash-cli select [节点名称/关键字]` | 交互选择节点，或通过关键字搜索直接切换 | `clash-cli select 日本` |
| `clash-cli update` | 更新订阅配置 | `clash-cli update` |
| `clash-cli ip` | 查询当前本地及代理后的公网 IP 地理位置 | `clash-cli ip` |
| `clash-cli log` | 查看并持续追踪实时日志 | `clash-cli log` |
| `clash-cli help` | 打印帮助信息 | `clash-cli help` |
