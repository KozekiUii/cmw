#include <cmw/transport/common/identity.h>

#include <random>

#include <cmw/common/util.h>

namespace hnu    {
namespace cmw   {
namespace transport {


/*UUID是一个128位的唯一标识符*/
Identity::Identity(bool need_generate) : hash_value_(0) {
  std::memset(data_, 0, ID_SIZE);
  if (need_generate) {
    // 随机设备产生种子
    std::random_device rd;
    // 创建离散均匀分布
    std::uniform_int_distribution<int> dist(0, 255);
    for (size_t i = 0; i < ID_SIZE; ++i) {
      // 使用随机数引擎 rd 产生一个随机值，并按该分布映射到目标区间
      data_[i] = static_cast<char>(dist(rd));
    }
    Update();
  }
}

/* 拷贝构造函数 */
Identity::Identity(const Identity& rhs) {
  std::memcpy(data_, rhs.data_, ID_SIZE);
  hash_value_ = rhs.hash_value_;
}


Identity::~Identity() {}

/*重载 = */
Identity& Identity::operator=(const Identity& rhs) {
  if (this != &rhs) {
    std::memcpy(data_, rhs.data_, ID_SIZE);
    hash_value_ = rhs.hash_value_;
  }
  return *this;
}


bool Identity::operator==(const Identity &rhs) const {
  // memcmp函数比较两个内存区域的前n个字节是否相同
  return std::memcmp(data_, rhs.data_, ID_SIZE) == 0;
}

bool Identity::operator!=(const Identity& rhs) const {
  return !(*this == rhs);
}


std::string Identity::ToString() const { return std::to_string(hash_value_); }

size_t Identity::Length() const { return ID_SIZE; }

uint64_t Identity::HashValue() const { return hash_value_; }

void Identity::Update() {
  // 将data_转换为std::string，为避免字符串截断，取前ID_SIZE个字符，并计算其哈希值，存储在hash_value_中
  hash_value_ = common::Hash(std::string(data_, ID_SIZE));
}


}
}
}

