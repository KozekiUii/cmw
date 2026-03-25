# 第 20 课：基本类型序列化的统一模式

## 一、本课目标

完成本课后，你应该能够：

1. 从多个具体函数中总结出 `DataStream` 处理基本类型的统一套路
2. 理解为什么基本类型写入通常采用“类型标识 + 数据值”的结构
3. 看懂基本类型读取时“先校验，再解析，再推进位置”的统一流程
4. 建立后续分析字符串、容器、自定义对象序列化的模板意识
5. 学会从重复代码里提炼设计规律，而不只是逐函数硬记

---

## 二、为什么这一课要专门讲“统一模式”

到这一课为止，你已经知道：
- `DataStream` 有缓冲区 `m_buf`
- 有读取位置 `m_pos`
- 有字节序 `m_byteorder`

接下来，如果你继续按函数一个个死读，很容易陷入一种低效状态：
- 看了 `write(bool)`
- 再看 `write(char)`
- 再看 `write(int32_t)`
- 再看 `write(double)`
- 感觉每个都不一样，但又好像差不多

这时最重要的能力，不是继续逐个背，而是：

> **从一组实现里提炼出共性。**

而当前 `DataStream` 里，基本类型序列化恰好非常适合做这种训练。

---

## 三、先明确：这里说的“基本类型”包括哪些

在当前仓库中，`DataStream` 支持的基本类型写入 / 读取主要包括：

- `bool`
- `char`
- `int32_t`
- `uint32_t`
- `int64_t`
- `uint64_t`
- `float`
- `double`

对应实现主要在 [serialize/data_stream.cpp](../serialize/data_stream.cpp) 中这些位置：

### 写入
- [serialize/data_stream.cpp:180-185](../serialize/data_stream.cpp#L180-L185)
- [serialize/data_stream.cpp:187-192](../serialize/data_stream.cpp#L187-L192)
- [serialize/data_stream.cpp:194-205](../serialize/data_stream.cpp#L194-L205)
- [serialize/data_stream.cpp:207-218](../serialize/data_stream.cpp#L207-L218)
- [serialize/data_stream.cpp:220-230](../serialize/data_stream.cpp#L220-L230)
- [serialize/data_stream.cpp:233-244](../serialize/data_stream.cpp#L233-L244)
- [serialize/data_stream.cpp:246-257](../serialize/data_stream.cpp#L246-L257)
- [serialize/data_stream.cpp:259-270](../serialize/data_stream.cpp#L259-L270)

### 读取
- [serialize/data_stream.cpp:308-317](../serialize/data_stream.cpp#L308-L317)
- [serialize/data_stream.cpp:320-329](../serialize/data_stream.cpp#L320-L329)
- [serialize/data_stream.cpp:332-348](../serialize/data_stream.cpp#L332-L348)
- [serialize/data_stream.cpp:351-367](../serialize/data_stream.cpp#L351-L367)
- [serialize/data_stream.cpp:369-385](../serialize/data_stream.cpp#L369-L385)
- [serialize/data_stream.cpp:387-403](../serialize/data_stream.cpp#L387-L403)
- [serialize/data_stream.cpp:405-421](../serialize/data_stream.cpp#L405-L421)
- [serialize/data_stream.cpp:423-439](../serialize/data_stream.cpp#L423-L439)

你现在的目标，不是把这些函数一个个背下来，而是找出它们共有的结构。

---

## 四、基本类型写入的第一层统一模式：先写类型，再写值

这是当前实现里最核心的统一规律。

### 1. 先看 `write(bool)`
见 [serialize/data_stream.cpp:180-185](../serialize/data_stream.cpp#L180-L185)。

它的模式是：
1. 写入 `DataType::BOOL`
2. 写入实际值

---

### 2. 再看 `write(char)`
见 [serialize/data_stream.cpp:187-192](../serialize/data_stream.cpp#L187-L192)。

也是：
1. 写入 `DataType::CHAR`
2. 写入实际值

---

### 3. 再看 `write(int32_t)`
见 [serialize/data_stream.cpp:194-205](../serialize/data_stream.cpp#L194-L205)。

依然是：
1. 写入 `DataType::INT32`
2. 根据字节序处理值
3. 写入实际值字节

---

### 4. 再看 `write(double)`
见 [serialize/data_stream.cpp:259-270](../serialize/data_stream.cpp#L259-L270)。

仍然是：
1. 写入 `DataType::DOUBLE`
2. 如有需要处理字节序
3. 写入值

所以这一组函数的最外层结构非常统一：

> **任何一个基本类型，都会先写入“我是什么类型”，再写入“我的具体内容”。**

这是当前实现非常重要的格式设计原则。

---

## 五、为什么要先写类型标识

这是理解当前序列化设计思路的关键。

### 1. 让数据流具有可解释性
如果流里只有一串裸字节，读取方就不知道：
- 当前位置是 `int32_t`
- 还是 `float`
- 还是 `char`

而一旦先写类型标识，解析方就能先判断“这段数据应该按什么规则解释”。

---

### 2. 让读取端可以做校验
你可以看读取函数，例如 [serialize/data_stream.cpp:332-336](../serialize/data_stream.cpp#L332-L336)：

```cpp
if (m_buf[m_pos] != DataType::INT32)
{
    return false;
}
```

这说明类型标识的意义不只是帮助理解，还能：

- 防止读错类型
- 尽早发现数据格式不匹配

---

### 3. 让连续读取成为可能
因为一个流里往往不是一个值，而是一串值。

例如你连续写入：
- 一个 `int32_t`
- 一个 `double`
- 一个 `bool`

那读取方就需要知道每一段的边界和解释方式。

类型标识正是在帮助系统建立这种“逐段解析”的能力。

---

## 六、基本类型写入的第二层统一模式：最终都落到原始字节写入

虽然各种 `write(T)` 表面上各不相同，但它们底层最终几乎都依赖：

- [serialize/data_stream.cpp:172-178](../serialize/data_stream.cpp#L172-L178)

也就是：

```cpp
void DataStream::write(const char * data, int len)
```

这说明上层所有基本类型写入，本质上都在做两件事：

1. 先把“类型 + 值”整理成字节
2. 再交给底层字节追加函数写进 `m_buf`

所以你可以把所有 `write(T)` 看成：

- **具体类型的编码逻辑**

而把 `write(const char*, int)` 看成：

- **统一的字节落地逻辑**

这是一种很常见的分层：
- 上层决定编码规则
- 下层负责字节搬运

---

## 七、基本类型写入的第三层统一模式：多字节类型要考虑字节序

并不是所有基本类型都完全一样。

这里要把它们再分成两类理解。

### 第一类：单字节类型
例如：
- `bool`
- `char`

对应实现：
- [serialize/data_stream.cpp:180-185](../serialize/data_stream.cpp#L180-L185)
- [serialize/data_stream.cpp:187-192](../serialize/data_stream.cpp#L187-L192)

这类类型长度只有一个字节，通常不需要字节序转换。

---

### 第二类：多字节类型
例如：
- `int32_t`
- `uint32_t`
- `int64_t`
- `uint64_t`
- `float`
- `double`

这类类型会在写入前根据 `m_byteorder` 做处理，例如：
- [serialize/data_stream.cpp:198-203](../serialize/data_stream.cpp#L198-L203)
- [serialize/data_stream.cpp:224-229](../serialize/data_stream.cpp#L224-L229)
- [serialize/data_stream.cpp:263-268](../serialize/data_stream.cpp#L263-L268)

所以这一层的统一规律是：

> **单字节类型直接写，多字节类型在写值前要先考虑字节序。**

这个区别是后面理解字符串长度、容器长度、枚举底层值时的重要前提。

---

## 八、基本类型读取的统一模式：先校验，再取值，再推进位置

如果说写入的统一模式是“先写类型，再写值”，那么读取的统一模式就是：

> **先检查类型标识，再读取值内容，最后推进 `m_pos`。**

这在几乎所有基本类型读函数里都能看到。

---

### 1. 先看 `read(bool & value)`
见 [serialize/data_stream.cpp:308-317](../serialize/data_stream.cpp#L308-L317)。

流程是：
1. 检查当前位置是不是 `BOOL`
2. `m_pos` 前进 1 格越过类型标识
3. 读取值
4. `m_pos` 再前进 1 格

---

### 2. 看 `read(char & value)`
见 [serialize/data_stream.cpp:320-329](../serialize/data_stream.cpp#L320-L329)。

也是完全一样的模板：
1. 检查类型
2. 越过类型位
3. 读取值
4. 推进位置

---

### 3. 看 `read(int32_t & value)`
见 [serialize/data_stream.cpp:332-348](../serialize/data_stream.cpp#L332-L348)。

流程依旧一致，只是中间多了字节序处理：
1. 检查当前位置是不是 `INT32`
2. `++m_pos`
3. 读取 4 字节值
4. 按需调整字节序
5. `m_pos += 4`

---

### 4. 看 `read(double & value)`
见 [serialize/data_stream.cpp:423-439](../serialize/data_stream.cpp#L423-L439)。

同样是：
1. 校验类型
2. 跳过类型标识
3. 读取 8 字节值
4. 处理字节序
5. 推进位置

所以当前实现的基本类型读取逻辑，可以抽象成一个非常稳定的模板。

---

## 九、把基本类型读写分别抽象成伪代码

这一部分非常重要。你应该学会把真实代码还原成抽象模板。

### 基本类型写入模板

```text
写入类型标识
如果值是多字节类型：
    根据字节序调整字节顺序
把值的字节写入缓冲区
```

---

### 基本类型读取模板

```text
检查当前位置的类型标识是否匹配
如果不匹配：返回失败
跳过类型标识
读取固定长度字节
如果值是多字节类型：
    根据字节序调整字节顺序
推进读取位置
返回成功
```

一旦你掌握了这两个模板，再看后面的字符串、枚举、容器，就会轻松很多。

---

## 十、为什么说这种统一模式很适合教学

当前实现虽然未必是最极致、最工业化的设计，但它非常适合学习，因为它的规律很清楚。

### 1. 重复足够明显
同一模式在多个函数中反复出现，非常容易训练你提炼共性的能力。

### 2. 结构足够直观
你能清楚看到：
- 类型标识
- 值字节
- 字节序处理
- 位置推进

这些序列化中的关键元素都没有被藏起来。

### 3. 容易迁移到后续内容
后面学：
- 字符串
- 容器
- 枚举
- 自定义对象

本质上都会继续用到类似思路，只是结构更复杂一点。

所以这一课实际上是在给你建立“读序列化代码的通用模板”。

---

## 十一、当前实现中你特别要注意的一个认知：读取逻辑不是“猜”，而是“验证”

这是序列化系统里非常重要的一种思想。

很多新手会把反序列化想成：
- 我知道这里应该是个 `int`，那我就直接取 4 字节好了

但当前实现不是这样。

它的模式是：
- 先验证当前位置的类型是不是我期待的
- 只有通过验证才继续取值

也就是说，这是一种：

> **先确认协议，再执行解析**

的思路。

这个思路非常重要，因为它意味着：
- 系统在尽量降低错读风险
- 流数据不是盲目解释，而是依据格式规则解析

你后面读字符串和容器实现时，也要继续保持这种观察角度。

---

## 十二、从这一课开始，你应该学会怎么“快速扫一组相似函数”

当你以后看到一组结构相似的函数时，可以按下面这个方法读：

### 第一步：找共同骨架
比如这一课的共同骨架就是：
- 写：类型 + 值
- 读：校验 + 取值 + 推进

### 第二步：找差异点
比如：
- 是否需要字节序处理
- 占用几个字节
- 是否需要额外长度信息

### 第三步：总结可复用模板
把这组函数压缩成几行伪代码。

这是一种非常适合学习中间件代码的阅读方式。

---

## 十三、本课小结

本课最核心的结论是：

> **`DataStream` 对基本类型的处理具有非常明确的统一模式：写入时通常是“类型标识 + 数据值”，读取时通常是“先校验类型，再解析值，最后推进位置”。**

你现在应该记住：

- 当前基本类型写入实现主要集中在 [serialize/data_stream.cpp:180-270](../serialize/data_stream.cpp#L180-L270)
- 当前基本类型读取实现主要集中在 [serialize/data_stream.cpp:308-439](../serialize/data_stream.cpp#L308-L439)
- 写入端的统一外层结构是“先写类型，再写值”
- 读取端的统一外层结构是“先校验，再取值，再推进 `m_pos`”
- 多字节类型相比单字节类型，多了一层字节序处理
- 所有高层写入最终都会落到底层字节写入函数 [serialize/data_stream.cpp:172-178](../serialize/data_stream.cpp#L172-L178)
- 学会提炼统一模式，比逐函数死记更重要

---

## 十四、本课练习

### 练习 1：自己总结基本类型写入模板
请你用不超过 4 行的话，总结：
- `DataStream` 写一个基本类型时，一般会经历哪些步骤？

---

### 练习 2：对比 `read(bool)` 与 `read(int32_t)`
请你分别回答：
- 它们的共同点是什么？
- 它们最大的差异点是什么？

提示：
- 重点看类型校验、位置推进、字节序处理。

---

### 练习 3：思考为什么“先写类型”是有价值的
请你回答：
- 如果序列化流里不写类型标识，会给反序列化带来什么问题？
- 至少写出 3 点。

---

## 十五、下一课预告

下一课我们会进入：

# 第 21 课：字符串序列化——为什么要额外记录长度

重点会看：
- `string` 和基本数值相比，为什么不能只写类型和内容
- 字符串序列化为什么必须引入长度信息
- `write(const char*)` 与 `write(const string&)` 的共同结构
- `read(string&)` 如何依赖长度信息恢复数据

也就是说，下一课会从“固定长度类型”过渡到“变长类型”的序列化思维。