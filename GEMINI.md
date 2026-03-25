# CMW (Cyber Middleware) 项目指南

CMW 是一个基于 Apollo Cyber RT 架构思想和 Fast-DDS 实现的高性能中间件项目，旨在为分布式系统（如自动驾驶、机器人）提供高效、实时的通信和任务调度能力。

## 项目概览

- **核心技术栈**: C++14, Fast-DDS (RTPS), 共享内存 (SHM), 协程调度 (CRoutine).
- **主要架构组件**:
    - **Node (节点)**: 核心通信实体，用于创建 Publisher (发布者) 和 Subscriber (订阅者)。
    - **Transport (传输层)**: 提供 RTPS 和 SHM 两种传输模式，支持高效的数据交换。
    - **Scheduler (调度器)**: 基于协程的任务调度引擎，支持多种调度策略（Classic/Choreography）。
    - **CRoutine (协程)**: 用户态轻量级线程，减少上下文切换开销。
    - **Discovery (发现机制)**: 基于 Topology Manager 实现节点和通道的自动发现。
    - **Component (组件)**: 提供组件化开发框架，方便功能模块的构建和集成。

## 目录结构

- `base/`: 基础数据结构和并发工具（原子哈希表、锁、队列等）。
- `node/`: 节点、发布者、订阅者的抽象与实现。
- `transport/`: 传输层实现，包含 RTPS 和共享内存传输。
- `scheduler/`: 调度器实现，管理任务分发和执行。
- `croutine/`: 协程框架，包含上下文切换逻辑。
- `common/`: 通用工具类、宏定义、日志系统。
- `discovery/`: 拓扑发现和管理。
- `component/`: 组件化开发接口。
- `example/`: 示例程序，展示了各项功能的用法。
- `conf/`: 配置文件，用于定义调度和系统参数。

## 构建与运行

项目主要通过 `Makefile` 进行构建。

### 依赖项
- **Fast-DDS**: 必须安装并配置好路径。
- **gtest**: 用于单元测试。
- **System**: Linux (建议), pthread, uuid, rt, atomic.

### 编译示例
进入 `example` 目录并运行 `make`:
```bash
cd example
make
```
编译生成的二进制文件后缀为 `.out`。

### 运行
例如运行发布/订阅测试：
```bash
# 启动订阅者
./test_subscriber.out
# 启动发布者
./test_publisher.out
```

## 开发规范

- **命名空间**: 所有的核心代码都位于 `hnu::cmw` 命名空间下。
- **单例模式**: 许多管理类（如 `Transport`, `Scheduler`, `TopologyManager`）使用 `DECLARE_SINGLETON` 宏实现。
- **日志**: 使用项目内置的日志宏（如 `AINFO`, `AERROR`, `ADEBUG`），定义在 `common/log.h`。
- **错误处理**: 倾向于使用宏（如 `RETURN_VAL_IF_NULL`）进行快速错误返回。
- **C++ 标准**: 项目使用 `C++14` 标准。

## 核心交互流

1. **初始化**: 调用 `hnu::cmw::Init()` 初始化系统环境。
2. **创建节点**: 使用 `hnu::cmw::CreateNode("node_name")` 创建通信节点。
3. **通信**: 通过节点创建 `Publisher` 发送数据，或创建 `Subscriber` 注册回调函数接收数据。
4. **调度**: 任务（Task）会被封装成 `CRoutine` 由 `Scheduler` 分发到 `Processor` 执行。
