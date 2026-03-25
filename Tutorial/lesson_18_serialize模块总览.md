# 第 18 课：`serialize` 模块总览

## 一、本课目标

完成本课后，你应该能够：

1. 从模块视角理解当前仓库 `serialize` 子系统的整体结构
2. 说清 `Serializable` 与 `DataStream` 各自承担什么职责
3. 理解这个模块为什么采用“接口 + 数据流对象”的组合
4. 看懂序列化主线接下来应该如何继续深入
5. 为后续精读 [serialize/data_stream.h](../serialize/data_stream.h) 与 [serialize/data_stream.cpp](../serialize/data_stream.cpp) 建立路线图

---

## 二、先看当前 `serialize` 模块有哪些文件

当前仓库中，`serialize` 目录下主要有这 3 个文件：

- [serialize/serializable.h](../serialize/serializable.h)
- [serialize/data_stream.h](../serialize/data_stream.h)
- [serialize/data_stream.cpp](../serialize/data_stream.cpp)

这说明目前的序列化模块规模不大，但结构已经比较清晰：

- 一个文件定义“可序列化对象”的接入接口
- 一个文件声明“数据流”的对外能力
- 一个文件实现“数据流”的具体行为

也就是说，这个模块已经具备了一个完整子系统的基本骨架。

---

## 三、如果只用一句话概括 `serialize` 模块

一个比较准确的说法是：

> **`serialize` 模块负责把对象编码到统一字节流中，并支持从字节流中按规则恢复对象。**

这句话里有两个关键词：

- **统一字节流**
- **按规则恢复**

前者强调输出形式统一，后者强调恢复过程可解释、可重复。

---

## 四、这个模块的核心角色有哪些

从代码上看，当前 `serialize` 模块可以拆成两个核心角色。

### 角色 1：`Serializable`
定义在 [serialize/serializable.h:10-15](../serialize/serializable.h#L10-L15)。

它的职责是：

- 规定“一个自定义对象怎样参与序列化 / 反序列化”

接口非常直接：
- `serialize(DataStream & stream) const`
- `unserialize(DataStream & stream)`

所以 `Serializable` 更像一份“接入协议”或“能力约定”。

它本身不负责存数据、不负责维护缓冲区，也不直接决定底层字节布局。

它主要回答的问题是：

> **自定义对象要如何把自己的内容交给序列化系统。**

---

### 角色 2：`DataStream`
定义在 [serialize/data_stream.h:27-203](../serialize/data_stream.h#L27-L203)，实现位于 [serialize/data_stream.cpp](../serialize/data_stream.cpp)。

它的职责是：

- 维护序列化后的字节缓冲区
- 提供各种类型的写入接口
- 提供各种类型的读取接口
- 管理读写位置
- 处理类型标识、长度信息、字节序等问题
- 支持把数据保存到文件、从文件中加载

也就是说，`DataStream` 才是这个模块真正的“执行中枢”。

如果说 `Serializable` 是“规则入口”，那么 `DataStream` 就是“实际运转的引擎”。

---

## 五、为什么这个模块不是只提供一堆 `serialize(x)` 函数

这是一个很值得思考的设计问题。

很多简单库会做成：
- `serialize(x)`
- `deserialize(buf, x)`

但当前仓库选择了一个**带状态的流对象**：`DataStream`。

从 [serialize/data_stream.h:200-202](../serialize/data_stream.h#L200-L202) 可以看到，它内部维护：

- `std::vector<char> m_buf`
- `int m_pos`
- `ByteOrder m_byteorder`

这表明作者的设计意图不是“单次函数转换”，而是：

> **围绕一段持续存在的字节流，组织多次写入、连续读取和状态推进。**

这样做的好处是：
- 多个字段可以连续写入到同一流中
- 读取时可以顺序恢复，不必每次都手动传偏移量
- 更接近中间件里的消息流、缓存流、文件流思维
- 操作体验更像流式接口，而不是离散工具函数

所以 `DataStream` 的存在，体现的是一种系统化设计，而不是零散工具化设计。

---

## 六、从接口层面看，`DataStream` 支持什么能力

这一课先做总览，不深入每个函数细节，但你要先建立能力地图。

### 1. 基本类型写入 / 读取
在 [serialize/data_stream.h:62-75](../serialize/data_stream.h#L62-L75) 和 [serialize/data_stream.h:101-112](../serialize/data_stream.h#L101-L112) 可以看到，支持：

- `bool`
- `char`
- `int32_t`
- `uint32_t`
- `int64_t`
- `uint64_t`
- `float`
- `double`
- `string`
- `Serializable`

这说明它首先想解决的是：

- 标准基础类型和简单对象能统一进入流

---

### 2. 容器类型写入 / 读取
在 [serialize/data_stream.h:77-88](../serialize/data_stream.h#L77-L88) 和 [serialize/data_stream.h:114-125](../serialize/data_stream.h#L114-L125) 可以看到，支持：

- `std::vector<T>`
- `std::list<T>`
- `std::map<K, V>`
- `std::set<T>`

这一步很关键。

因为一旦支持容器，序列化系统就不只是“传单个值”，而是能够承载更复杂的对象结构。

这也意味着 `DataStream` 已经开始承担“复合数据组织”的任务。

---

### 3. 枚举类型支持
在 [serialize/data_stream.h:90-93](../serialize/data_stream.h#L90-L93) 和 [serialize/data_stream.h:127-130](../serialize/data_stream.h#L127-L130) 中，作者通过模板约束支持枚举类型。

这说明模块希望：
- 一部分用户自定义类型不必每次都完全手写转换
- 枚举可以被视为其底层整数表示处理

这提升了易用性。

---

### 4. 多参数读写支持
在 [serialize/data_stream.h:95-100](../serialize/data_stream.h#L95-L100) 与 [serialize/data_stream.h:132-136](../serialize/data_stream.h#L132-L136) 中，`write_args` / `read_args` 提供了可变参数支持。

再结合 [serialize/serializable.h:17-36](../serialize/serializable.h#L17-L36) 中的 `SERIALIZE(...)` 宏，你会发现：

- 自定义对象可以很方便地把多个成员依次写入
- 再按相同顺序读回

这说明模块设计不仅在乎“能做”，还在乎“怎么更方便地做”。

---

### 5. 文件保存 / 加载
在 [serialize/data_stream.h:144-145](../serialize/data_stream.h#L144-L145) 和 [serialize/data_stream.cpp:498-517](../serialize/data_stream.cpp#L498-L517) 中，`DataStream` 还支持：

- `save`
- `load`

这意味着这套字节流不仅可用于内存中的消息交换，也可直接用于文件存取。

所以它连接了两个世界：
- 内存中的对象世界
- 外部存储 / 交换世界

---

### 6. 类流式运算符接口
在 [serialize/data_stream.h:147-193](../serialize/data_stream.h#L147-L193) 和 [serialize/data_stream.cpp:519-642](../serialize/data_stream.cpp#L519-L642) 中，还提供了 `<<` 与 `>>`。

这进一步说明它希望把使用体验做成：

- 更接近 C++ 流式写法
- 更适合连续读写多个字段

因此，`DataStream` 在 API 风格上也比较统一。

---

## 七、`Serializable` 和 `DataStream` 是如何协作的

这是本课的核心。

### 1. `DataStream` 负责“怎么写、怎么读”
例如：
- 类型标记如何写入
- 长度如何记录
- 字节怎么放进缓冲区
- 读取时游标如何移动

这些都属于 `DataStream` 的职责。

---

### 2. `Serializable` 负责“对象有哪些字段要参与”
例如一个自定义类里可能有：
- `id`
- `name`
- `scores`

那么这个类自己决定：
- 以什么顺序把这些字段交给 `DataStream`
- 以什么顺序读回来

也就是说，自定义对象知道“内容”，`DataStream` 知道“搬运规则”。

---

### 3. 两者组合后形成完整序列化路径
完整过程可以概括为：

1. 对象调用 `serialize(stream)`
2. 对象把成员逐个交给 `stream`
3. `stream` 按自己的编码规则写入字节缓冲区
4. 需要恢复时，调用 `unserialize(stream)`
5. `stream` 按规则顺序读出
6. 对象把读出的值重新填回自己的成员

所以这是一种非常典型的：

- **对象描述内容**
- **流对象执行编码 / 解码**

的合作模式。

---

## 八、从当前实现里，可以看到 `DataStream` 的底层主轴是什么

你可以把 `DataStream` 的内部主轴概括成 3 个关键词。

### 1. 缓冲区：`m_buf`
见 [serialize/data_stream.h:200](../serialize/data_stream.h#L200)。

所有序列化结果最终都进入这段 `std::vector<char>`。

这说明：
- 字节流的物理承载体就是这一段连续字符缓冲区
- 无论基本类型、字符串、容器还是对象，最终都要落到这里

---

### 2. 位置：`m_pos`
见 [serialize/data_stream.h:201](../serialize/data_stream.h#L201)。

它表示当前读位置。

从 [serialize/data_stream.cpp:301-305](../serialize/data_stream.cpp#L301-L305)、[serialize/data_stream.cpp:332-348](../serialize/data_stream.cpp#L332-L348) 等实现可以看出：
- 读取发生后，`m_pos` 会不断向后推进
- `reset()` 可以把位置回到起点，见 [serialize/data_stream.cpp:487-490](../serialize/data_stream.cpp#L487-L490)

这就是为什么 `DataStream` 是一个带状态的“流对象”。

---

### 3. 字节序：`m_byteorder`
见 [serialize/data_stream.h:202](../serialize/data_stream.h#L202)。

初始化时通过 [serialize/data_stream.cpp:60-74](../serialize/data_stream.cpp#L60-L74) 检测当前系统字节序。

再在基本数值写入 / 读取时做字节翻转处理，例如：
- [serialize/data_stream.cpp:194-205](../serialize/data_stream.cpp#L194-L205)
- [serialize/data_stream.cpp:332-348](../serialize/data_stream.cpp#L332-L348)

这说明当前实现已经意识到：
- 底层数值表示必须考虑跨平台一致性问题

虽然后续我们还会分析它的具体处理是否严谨，但从模块总览层面，至少能确认：

**作者把字节序作为序列化系统的一部分来考虑了。**

---

## 九、这个模块的编码规则，大致是什么风格

从目前代码可以看出，当前 `DataStream` 的风格是：

> **以类型标识开头，再跟对应数据内容；对于复合类型，通常还会写入长度信息。**

例如：

### 1. 基本类型
以 `int32_t` 为例：
- 先写一个 `DataType::INT32`
- 再写 4 字节值

见 [serialize/data_stream.cpp:194-205](../serialize/data_stream.cpp#L194-L205)。

---

### 2. 字符串
以 `string` 为例：
- 先写 `DataType::STRING`
- 再写长度
- 再写字符串内容

见 [serialize/data_stream.cpp:281-288](../serialize/data_stream.cpp#L281-L288)。

---

### 3. 容器
以 `vector<T>` 为例：
- 先写 `DataType::VECTOR`
- 再写元素个数
- 再写元素内容

见 [serialize/data_stream.h:205-227](../serialize/data_stream.h#L205-L227)。

这说明模块整体采用的是一种：

- **先描述，再承载内容**

的格式设计思想。

这也是为什么读取时能够先检查类型，再决定如何解析。

---

## 十、当前实现为什么值得你重点学

当前 `serialize` 模块很适合作为学习材料，因为它同时具备两个特点：

### 1. 结构清楚
文件不多，主线明确。

你很容易在较短时间内搞清：
- 模块角色
- 接口关系
- 核心数据结构
- 基本数据流转路径

---

### 2. 有真实设计取舍
虽然它规模不大，但并不是“玩具函数”。

你已经可以看到很多中间件序列化设计里都会遇到的问题：
- 类型标识
- 字节序
- 容器编码
- 自定义对象接入
- 多字段连续编码
- 文件读写
- 流式接口设计

所以学这个模块，不只是学一份代码，而是在练一套系统化观察方法。

---

## 十一、学习这个模块时，不要只盯“怎么写”，更要盯“为什么这么分层”

你接下来精读 `serialize` 模块时，很容易把注意力全部放在：
- 某个函数怎么实现
- 某个类型怎么编码

但更高价值的问题是：

### 1. 为什么单独抽出 `Serializable`
因为自定义对象的内容组织应该由对象自己决定。

### 2. 为什么单独抽出 `DataStream`
因为底层编码 / 缓冲区 / 位置推进 / 文件读写等通用能力不该散落在每个对象里。

### 3. 为什么两者不合并
因为“描述对象内容”和“执行序列化操作”是两种不同层次的职责。

当你能看出这种分层理由时，你就不只是“看懂代码”，而是在理解作者的架构判断。

---

## 十二、当前 `serialize` 模块的学习顺序应该怎么安排

从第 18 课开始，后面的学习建议按下面顺序推进。

### 第一步：继续精读 `DataStream` 的整体接口
重点看：
- 它支持哪些类型
- 哪些能力属于读写核心
- 哪些能力属于辅助功能

---

### 第二步：重点搞清内部状态
重点看：
- `m_buf`
- `m_pos`
- `m_byteorder`

因为它们决定了这套系统最核心的运行方式。

---

### 第三步：按数据类别分开学
不要乱跳，建议按顺序看：
1. 基本类型
2. 字符串
3. 容器
4. 枚举
5. 自定义对象
6. 可变参数

这样更容易建立稳定心智模型。

---

### 第四步：最后看辅助能力
例如：
- `show`
- `save`
- `load`
- 运算符重载

这些是锦上添花，但不是最底层主干。

---

## 十三、这一课之后你应该形成的模块地图

现在你可以把 `serialize` 模块画成下面这张简单地图：

### 1. `Serializable`
负责：
- 定义自定义对象如何接入序列化系统

### 2. `DataStream`
负责：
- 承载字节缓冲区
- 写入各种数据
- 读取各种数据
- 管理读位置
- 处理格式规则
- 支持保存和加载

### 3. 两者关系
- `Serializable` 调用 `DataStream` 来完成对象字段的编码 / 解码
- `DataStream` 为 `Serializable` 提供统一底层能力

如果你脑中已经有这张图，后面精读时就不容易迷失。

---

## 十四、本课小结

本课最核心的结论是：

> **当前仓库的 `serialize` 模块采用了“对象接入接口 + 带状态数据流”的结构，让自定义对象描述内容，让 `DataStream` 负责实际编码、解码和缓冲管理。**

你现在应该记住：

- 当前 `serialize` 模块主要由 [serialize/serializable.h](../serialize/serializable.h)、[serialize/data_stream.h](../serialize/data_stream.h)、[serialize/data_stream.cpp](../serialize/data_stream.cpp) 构成
- `Serializable` 负责自定义对象接入规则
- `DataStream` 负责底层字节流组织和实际读写
- `DataStream` 内部的主轴是缓冲区、位置和字节序
- 当前模块已经支持基础类型、字符串、容器、枚举、自定义对象、多参数读写、文件保存与加载
- 后续学习的主轴应该转向 `DataStream` 的内部状态和具体编码规则

---

## 十五、本课练习

### 练习 1：写出 `Serializable` 和 `DataStream` 的职责分工
要求：
- 每个角色用一句话说明职责
- 再用一句话说明它们为什么不能混在一起

---

### 练习 2：画一张最小协作图
请你自己画出下面这个过程：

- 一个自定义对象
- 调用 `serialize(stream)`
- 把字段交给 `DataStream`
- 最终进入字节缓冲区
- 再通过 `unserialize(stream)` 恢复

不要求漂亮，只要求把数据流方向画清楚。

---

### 练习 3：从 `DataStream` 接口中分组
请你阅读 [serialize/data_stream.h](../serialize/data_stream.h)，把所有公开接口按下面几类分组：
- 基本类型读写
- 容器读写
- 自定义对象读写
- 可变参数支持
- 状态管理
- 文件操作
- 运算符接口

这个练习会帮助你快速熟悉接口层结构。

---

## 十六、下一课预告

下一课我们会进入：

# 第 19 课：`DataStream` 的核心状态——缓冲区、位置与字节序

重点会看：
- `m_buf` 到底在承载什么
- `m_pos` 为什么是理解整个读取流程的关键
- `m_byteorder` 为什么和跨平台数据表示相关
- 为什么这三个成员几乎决定了整个 `DataStream` 的基本行为

也就是说，下一课会开始真正深入 `DataStream` 的内部主轴。