#ifndef CMW_SERIALIZ_SERIALIZABLE_H_
#define CMW_SERIALIZ_SERIALIZABLE_H_

namespace hnu{
namespace cmw{
namespace serialize{

class DataStream;

class Serializable
{
public:
    virtual void serialize(DataStream & stream) const = 0;
    virtual bool unserialize(DataStream & stream) = 0;
};

// __VA_ARGS__ 是一个预定义的宏，代表可变参数列表
#define SERIALIZE(...)                              \
                                                    \
    void serialize(DataStream & stream) const       \
    {                                               \
        char type = DataStream::CUSTOM;             \
        stream.write((char *)&type, sizeof(char));  \
        stream.write_args(__VA_ARGS__);             \
    }                                               \
                                                    \
    bool unserialize(DataStream & stream)           \
    {                                               \
        char type;                                  \
        stream.read(&type, sizeof(char));           \
        if (type != DataStream::CUSTOM)             \
        {                                           \
            return false;                           \
        }                                           \
        stream.read_args(__VA_ARGS__);              \
        return true;                                \
    }

}
}
}


#endif
