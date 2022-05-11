#pragma once

#include <memory>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include "system.hpp"

__OBELISK__

class ByteArray{
public:
    typedef std::shared_ptr<ByteArray> ptr;

    struct Node{
        Node();
        Node(size_t s);
        ~Node();

        char* data;
        Node* next;
        size_t size;
    };

    /**
     * @param[in] size 链表长度
     */
    ByteArray(size_t basesize = 4096);
    ~ByteArray();

    /**
     * @brief int类型无压缩的写入方法
     */
    template<typename INT>
    void write(INT v){
        if(BYTE_ORDER != m_endian)
            v = byteswap(v);
        write(&v, sizeof(v));
    }
    
    void writeFint8(int8_t v);
    void writeFuint8( uint8_t v);
    void writeFint16( int16_t v);
    void writeFuint16( uint16_t v);
    void writeFint32( int32_t v);
    void writeFuint32( uint32_t v);
    void writeFint64( int64_t v);
    void writeFuint64( uint64_t v);
    /**
     * @brief 带压缩的写入方法
     */
    void writeInt32( int32_t v);
    void writeUint32( uint32_t v);
    void writeInt64( int64_t v);
    void writeUint64( uint64_t v);

    void writeFloat( float v);
    void writeDouble( double v);
    void writeStringF16(const std::string& v);
    void writeStringF32(const std::string& v);
    void writeStringF64(const std::string& v);
    void writeStringVint(const std::string& v);
    void writeStringWithoutLength(const std::string& v);

    int8_t   readFint8();
    uint8_t  readFuint8();
    int16_t  readFint16();
    uint16_t readFuint16();
    int32_t  readFint32();
    uint32_t readFuint32();
    int64_t  readFint64();
    uint64_t readFuint64();

    int32_t readInt32();
    uint32_t readUint32();
    int64_t readInt64();
    uint64_t readUint64();

    float readFloat();
    double readDouble();

    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringVint();

    void clear();

    void write(const void* buf, size_t size);
    void read(void* buf, size_t size);
    void read(void* buf, size_t size, size_t position) const;

    size_t getPosition() const { return m_position; }
    void setPosition(size_t v);

    bool writeToFile(const std::string& name) const;
    bool readFromFile(const std::string& name);

    size_t getBaseSize() const { return m_basesize; }
    size_t getReadSize() const { return m_size - m_position; }

    bool isLittleEndian() const;
    void setIsLittleEndian(bool val);

    std::string toString() const;
    std::string toHexString();

    int64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0u) const;
    int64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;
    /**
     *@brief 增加容量，不改变position 
     */
    int64_t getWriteBuffers(std::vector<iovec>& bufers, uint64_t len);

    size_t getSize() const { return m_size; }
private:
    void addCapacity(size_t size);
    size_t getCapacoty() const { return m_capacity - m_position; }
private:
    size_t m_basesize;      // 单节点存储的数据长度
    size_t m_position;      // 当前读取位置
    size_t m_capacity;      // 全部结点的总数据长度   
    size_t m_size;          // 链表长度
    int m_endian;

    Node* m_root;
    Node* m_current;

};
__END__