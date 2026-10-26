# ATM Qt

一个基于 **Qt 6.2.4 / C++11 / SQLite** 的 ATM 教学项目，用于练习多线程消息传递、状态机建模、桌面 GUI 和持久化数据访问。项目同时提供 Qt Widgets 图形界面和 CLI 终端入口，适合作为 C++/Qt 并发编程方向的课程设计或简历项目展示。

## 项目亮点

- **Actor 风格并发架构**：ATM、银行和显示设备分别运行在独立线程中，通过类型安全的消息队列通信，降低共享状态和线程耦合。
- **完整 ATM 流程**：插卡、PIN 校验、余额查询、取款、余额不足处理、取消交易和退卡。
- **SQLite 持久化**：账户、PIN、余额和管理员标记存储在 SQLite；首次启动自动创建表结构和演示账户。
- **Qt Widgets 管理后台**：管理员可查看账户列表，修改普通账户余额和账户 PIN，并创建或删除普通账户。
- **双入口运行**：Qt 图形界面用于演示交互流程，CLI 入口便于终端验证。
- **测试与诊断配置**：CMake 支持 GoogleTest、AddressSanitizer（ASan）和 ThreadSanitizer（TSan）构建选项。

## 技术栈

| 类别 | 技术 |
| --- | --- |
| UI | Qt Widgets 6.2.4 |
| 语言标准 | C++11 |
| 构建系统 | CMake 3.16+ |
| 数据库 | SQLite，通过 Qt SQL 访问 |
| 并发模型 | `std::thread` + 类型擦除消息队列 + dispatcher |
| 测试 | GoogleTest |
| 平台 | Windows（Visual Studio 2022）；代码结构可扩展到 Linux |

## 架构概览

```text
Qt Widgets / CLI  -->  ATM Actor  <-->  Bank Actor  -->  SQLite
                           |
                           v
                    UI Actor / display sink
```

- `libs/messaging`：线程安全队列、sender/receiver 和 dispatcher。
- `core/actors`：ATM、银行和 UI actor，负责业务状态和消息处理。
- `core/messages`：ATM、银行和 UI 之间的消息协议。
- `core/bank_database.*`：SQLite 仓储，负责建表、认证、余额查询、原子扣款和账户管理。
- `app`：Qt 主窗口、管理员窗口、显示桥接层和 CLI 入口。
- `tests`：消息队列、dispatcher、sender/receiver 以及显示输出测试。

## 构建

建议使用纯 ASCII 路径，以避免 Qt `moc` 在部分 Windows 环境下处理中文路径时出现问题。

### Windows + Visual Studio 2022

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="D:/Qt/6.2.4/msvc2019_64"
cmake --build build --config Debug
```

如果使用 MinGW 或其他 Qt 安装目录，只需调整 `CMAKE_PREFIX_PATH`。Qt 安装需要包含 `Core`、`Widgets` 和 `Sql` 模块，以及 SQLite 驱动（`qsqlite`）。Windows 构建在可用时会自动调用 `windeployqt` 部署运行库和 SQLite 插件。

构建目标：

- `atm_qt`：Qt Widgets 图形界面。
- `atm_cli`：终端交互程序。
- `messaging_tests`：GoogleTest 单元测试（检测到 GoogleTest 或开启下载选项时生成）。

### 测试与 Sanitizer

```powershell
# 使用已安装的 GoogleTest
cmake -S . -B build-tests -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="D:/Qt/6.2.4/msvc2019_64"
cmake --build build-tests --config Debug
ctest --test-dir build-tests -C Debug --output-on-failure

```

若未安装 GoogleTest，可在配置时增加 `-DENABLE_GTEST_FETCH=ON`（需要网络）。`ENABLE_ASAN=ON` 可用于 MSVC 或 GCC/Clang；`ENABLE_TSAN=ON` 仅适用于 GCC/Clang，不支持 MSVC。不同工具链需使用对应的 Qt 安装目录重新配置。

## 运行

首次运行会在当前目录创建 `atm.sqlite3`。也可以通过 `ATM_DB_PATH` 指定数据库路径：

```powershell
$env:ATM_DB_PATH = "$PWD\atm-demo.sqlite3"
.\build\Debug\atm_qt.exe
```

演示账户：

| 账户 | PIN | 初始余额 | 权限 |
| --- | --- | ---: | --- |
| `acc1234` | `1937` | 199 | 普通账户 |
| `admin` | `2468` | 0 | 管理员 |

演示账号和明文 PIN 仅用于本地教学与功能演示，不应直接用于生产系统。项目当前重点是并发模型、消息协议和 GUI/数据库集成，未实现生产级密码哈希、权限审计或远程服务隔离。

### CLI 快捷键

1. 输入 `i`，再输入银行卡号。
2. 输入 4 位 PIN；可以逐位输入后按 `p`，也可以直接输入完整的 4 位数字。
3. 登录后输入 `b` 查询余额，输入 `w` 并提供正整数金额取款，输入 `c` 取消或退卡。
4. 输入 `q` 退出程序。

## 简历描述参考

> 使用 Qt 6.2.4、C++11 和 CMake 开发多线程 ATM 教学系统；基于 actor 风格消息队列拆分 ATM、银行和 UI 组件，使用 SQLite 完成账户持久化与原子扣款，提供 Qt Widgets 管理后台和 CLI 双入口；为消息队列和显示输出编写 GoogleTest 测试，并配置 ASan/TSan 构建选项。

## 目录结构

```text
.
├── app/                    # Qt GUI、管理员窗口和 CLI 入口
├── core/
│   ├── actors/             # ATM、Bank、UI actors
│   ├── messages/           # 消息协议
│   └── bank_database.*     # SQLite 账户仓储
├── libs/messaging/         # 消息队列与 dispatcher
├── tests/                  # GoogleTest 测试
├── CMakeLists.txt
└── verify.ps1              # 编码、路径和依赖检查脚本
```

## License

This project is released under the [MIT License](LICENSE), allowing anyone to use, copy, modify, publish, distribute, sublicense, and sell the software, subject to the license terms.
