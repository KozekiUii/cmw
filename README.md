# cmw

`cmw` 是一个参考 Cyber RT 思路、结合 Fast DDS 与共享内存传输实现的 C++ 中间件实验项目。仓库当前包含传输层、发布订阅抽象、拓扑发现、配置解析、序列化、调度相关基础设施，以及一组可直接运行的示例程序。

## 项目目标

- 提供统一的发布/订阅通信接口
- 针对不同通信场景支持多种传输方式
- 为节点、组件、调度与拓扑发现预留上层抽象
- 用较小的代码体量梳理中间件关键模块的设计思路

## 核心能力

- `Transport` 抽象统一封装发送端、接收端与底层分发逻辑
- 支持 `RTPS` 与 `SHM` 两种主要通信方式
- `Node` / `Publisher` / `Subscriber` 提供上层接口
- `RoleAttributes`、`TopologyManager` 等模块负责拓扑管理与角色发现
- `serialize` 模块提供简单序列化能力
- `config` 模块通过 JSON 配置调度与通信参数

## 仓库结构

```text
cmw/
├─ base/         基础工具与并发原语
├─ common/       全局环境、日志、文件与公共辅助函数
├─ component/    组件抽象
├─ config/       配置结构与解析逻辑
├─ discovery/    拓扑发现与角色管理
├─ doc/          模块设计说明
├─ example/      示例程序与示例构建入口
├─ node/         Node / Publisher / Subscriber 抽象
├─ scheduler/    调度相关实现
├─ serialize/    序列化模块
├─ transport/    RTPS / SHM / Dispatcher / Receiver / Transmitter
└─ conf/         运行所需配置文件
```

## 依赖环境

- Linux / 类 Unix 环境
- `g++`，支持 `-std=c++14`
- [Fast DDS](https://fast-dds.docs.eprosima.com/) 及其依赖库
- 可选：`gtest`，用于部分示例/测试目标

仓库中的 `example/Makefile` 默认假设目录结构如下：

```text
<workspace>/
├─ cmw/
└─ Fast-DDS/install/
```

也就是说，如果仓库位于 `<workspace>/cmw`，则默认会从 `<workspace>/Fast-DDS/install` 查找 Fast DDS。你也可以通过环境变量覆盖：

```bash
export FASTDDS_HOME=/path/to/Fast-DDS/install
export GTEST_HOME=/path/to/gtest
```

## 配置说明

运行时会读取 `conf/cmw.pb.conf`。项目默认通过环境变量 `CMW_PATH` 查找工作根目录：

```bash
export CMW_PATH=/absolute/path/to/cmw
```

如果未设置，代码会回退到 `/cmw`，这通常会导致本地开发环境下找不到配置文件。

仓库已提供一组配置样例，位于 `conf/` 目录，例如：

- `conf/cmw.pb.conf`
- `conf/example_sched.conf`
- `conf/example_sched_classic.conf`
- `conf/example_sched_choreography.conf`

## 快速开始

### 1. 准备依赖

确保 Fast DDS 已安装，并设置好 `FASTDDS_HOME` 或按默认目录放置。

### 2. 设置工作目录

```bash
export CMW_PATH=/absolute/path/to/cmw
```

### 3. 编译示例

```bash
cd example
make
```

如果系统中检测不到 `gtest`，`Makefile` 会自动跳过依赖 gtest 的目标，只编译基础示例。

### 4. 运行一个最小通信示例

终端 1：

```bash
cd example
./test_writer.out
```

终端 2：

```bash
cd example
./test_reader.out
```

这两个示例会构造 `RoleAttributes`，创建 `Transport` 层的发送端/接收端，并基于示例消息演示基本通信流程。

## 主要示例

- `example/test_writer.cpp`：发送端示例
- `example/test_reader.cpp`：接收端示例
- `example/test_serialize.cpp`：序列化示例
- `example/test_publisher.cpp`：发布者示例
- `example/test_subscriber.cpp`：订阅者示例
- `example/test_channel_manager.cpp`：拓扑/通道管理示例

## 文档索引

仓库中的设计说明主要放在 `doc/`：

- `doc/transport.md`：传输层设计、RTPS 与共享内存通信流程
- `doc/topology.md`：拓扑发现与角色管理
- `doc/scheduler.md`：调度模块设计
- `doc/serialize.md`：序列化模块说明
- `doc/fastrtps.md`：Fast RTPS 相关笔记
- `doc/croutine.md`：协程模块说明

## 当前状态

这个仓库目前更适合作为：

- 中间件设计与实现的学习项目
- 通信框架原型验证仓库
- Cyber RT / DDS / SHM 相关机制的拆解与实验场

相比一个完整可发布的中间件 SDK，当前仓库仍保留了较多实验性质内容，例如：

- 构建入口尚未完全统一
- 部分模块仍在演进中
- 示例、文档与实现之间还存在继续整理空间

如果你准备继续完善这个项目，建议优先从以下路径阅读：

1. `example/` 了解最小使用方式
2. `transport/` 理解核心通信链路
3. `node/` 与 `component/` 理解上层封装
4. `doc/` 对照设计说明阅读源码
