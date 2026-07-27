# Mihomo (Clash Meta) CLI Manager

这是一个使用 **C++17** 编写的命令行管理工具，用于便捷管理和控制本地 Mihomo (Clash Meta) 服务。它提供高性能的交互式终端 TUI 界面与快捷命令行指令。

---

## 目录结构 (Project Directory Structure)

项目结构设计清晰，便于维护：

```text
clash-cli/
├── CMakeLists.txt              # CMake 构建配置文件
├── include/                    # 头文件目录
│   └── clash-cli/
│       ├── api.hpp             # REST API 客户端与 IP 查询
│       ├── config.hpp          # 配置解析与订阅管理
│       ├── service.hpp         # 系统服务控制 (启动/停止/日志)
│       └── tui.hpp             # 终端 TUI 功能 (Spinner/键输入捕获/搜索选择列表)
├── src/                        # 源文件目录
│   ├── api.cpp                 # API 请求与网络查询实现
│   ├── config.cpp              # 配置文件解析与订阅持久化实现
│   ├── main.cpp                # 命令行入口与主菜单循环
│   ├── service.cpp             # 进程管理与脚本封装实现
│   └── tui.cpp                 # TUI 核心动画与按键交互实现
└── third_party/                # 三方依赖库 (Header-only)
    ├── httplib/
    │   └── httplib.h           # Header-only 高性能 HTTP 客户端
    └── nlohmann/
        └── json.hpp            # Header-only JSON 解析库
```

---

## 功能特性

1. **高性能与低开销**：基于 C++17 原生编译，瞬间启动，占用系统资源极少。
2. **一键服务管理**：快捷启动、停止、重启代理服务。
3. **实时状态显示**：在终端实时展示当前服务的运行状态、PID、路由模式以及各策略组的节点。
4. **交互式订阅管理**：
   - 支持多订阅管理：选择生效订阅、增加订阅、删除订阅。
   - 在管理菜单中直接更新订阅配置（一键调用后台脚本拉取更新）。
5. **交互式节点切换**：
   - 自动获取所有可用代理节点，以多列精美对齐的菜单提供选择。
   - **支持搜索过滤**：在切换节点时输入关键字即可进行模糊搜索，极速锁定目标节点。
6. **命令行快捷指令**：
   - 支持快捷命令 `clash-cli select <节点名称>` 快速搜索并切换节点。
   - 支持快捷命令 `clash-cli mode <rule/global/direct>` 快速修改运行模式。
7. **实时日志查看**：集成日志监听，使用 `Ctrl + C` 即可随时安全退出日志监听返回主菜单。
8. **外网 IP 及归属地查询**：同时查询本地直连公网 IP 与代理后公网 IP，快速确认代理生效状态及其地理位置。

---

## 编译与安装 (Build & Install)

### 1. 编译项目
请在 `clash-cli` 目录下使用标准 CMake 命令进行编译：

```bash
cd clash-cli
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

编译成功后，将在 `build` 目录下生成可执行程序 `clash-cli`。

### 2. 配置环境变量 / 创建软链接
为了能够在系统任何路径下直接运行 `clash-cli`，可以创建软链接到您的个人环境变量目录 `~/.local/bin` 中：

```bash
ln -sf /nfs/guokezhen/cli/clash-cli/build/clash-cli /home/skh_gkz/.local/bin/clash-cli
```

---

## 命令行指令说明

除了直接运行 `clash-cli` 进入终端交互控制台之外，您还可以使用以下快捷命令：

| 命令 | 说明 | 示例 |
| :--- | :--- | :--- |
| `clash-cli` | 打开交互控制台（默认） | `clash-cli` |
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
