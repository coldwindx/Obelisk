#include <string.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "log.h"
#include "endian.hpp"
#include "bytearray.h"

__OBELISK__

static Logger::ptr g_logger = LOG_SYSTEM();

ByteArray::Node::Node() 
        : data(nullptr), size(0), next(nullptr){

}
ByteArray::Node::Node(size_t s) 
        : data(new char[s]), size(s), next(nullptr){

}
ByteArray::Node::~Node(){
    if(data)
        delete[] data;
}


ByteArray::ByteArray(size_t basesize)
        : m_basesize(basesize), m_position(0), m_capacity(basesize)
        , m_size(0), m_endian(BIG_ENDIAN)
        , m_root(new Node(basesize)), m_current(m_root){
}

ByteArray::~ByteArray(){
    Node* tmp = m_root;
    while(tmp){
        m_current = tmp;
        tmp = tmp->next;
        delete m_current;
    }
}

void ByteArray::writeFint8(int8_t v){
    write(&v, sizeof(v));
}
void ByteArray::writeFuint8( uint8_t v){
    write(&v, sizeof(v));
}
void ByteArray::writeFint16( int16_t v){
    if(BYTE_ORDER != m_endian)
        v = byteswap(v);
    write(&v, sizeof(v));
}
void ByteArray::writeFuint16(uint16_t v){
    if(BYTE_ORDER != m_endian)
        v = byteswap(v);
    write(&v, sizeof(v));
}
void ByteArray::writeFint32( int32_t v){
    if(BYTE_ORDER != m_endian)
        v = byteswap(v);
    write(&v, sizeof(v));
}
void ByteArray::writeFuint32( uint32_t v){
    if(BYTE_ORDER != m_endian)
        v = byteswap(v);
    write(&v, sizeof(v));
}
void ByteArray::writeFint64( int64_t v){
    if(BYTE_ORDER != m_endian)
        v = byteswap(v);
    write(&v, sizeof(v));
}
void ByteArray::writeFuint64( uint64_t v){
    if(BYTE_ORDER != m_endian)
        v = byteswap(v);
    write(&v, sizeof(v));
}

static uint32_t EncodeZigzag32(const int32_t& v){
    if(v < 0)
        return ((uint32_t)(-v)) * 2 - 1;
    return v * 2;
}
static uint32_t EncodeZigzag64(const int64_t& v){
    if(v < 0)
        return ((uint64_t)(-v)) * 2 - 1;
    return v * 2;
}
static int32_t DecodeZigzag32(const uint32_t & v){
    return (v >> 1) ^ -(v & 1);
}
static int32_t DecodeZigzag64(const uint64_t & v){
    return (v >> 1) ^ -(v & 1);
}
void ByteArray::writeInt32( int32_t v){
    writeUint32(EncodeZigzag32(v));
}
void ByteArray::writeUint32( uint32_t v){
    uint8_t tmp[5];
    uint8_t i = 0;
    while(0x80 <= v){
        tmp[i++] = (v & 0x7f) | 0x80;
        v >>= 7;
    }
    tmp[i++] = v;
    write(tmp, i);
}

void ByteArray::writeInt64( int64_t v){
    writeUint64(EncodeZigzag64(v));
}
void ByteArray::writeUint64( uint64_t v){
    uint8_t tmp[10];
    uint8_t i = 0;
    while(0x80 <= v){
        tmp[i++] = (v & 0x7f) | 0x80;
        v >>= 7;
    }
    tmp[i++] = v;
    write(tmp, i);
}

void ByteArray::writeFloat( float v){
    uint32_t value;
    memcpy(&value, &v, sizeof(v));
    writeFuint32(v);
}
void ByteArray::writeDouble( double v){
    uint64_t value;
    memcpy(&value, &v, sizeof(v));
    writeFuint64(v);
}
void ByteArray::writeStringF16(const std::string& v){
    writeFuint16(v.size());
    write(v.c_str(), v.size());
}
void ByteArray::writeStringF32(const std::string& v){
    writeFuint32(v.size());
    write(v.c_str(), v.size());
}
void ByteArray::writeStringF64(const std::string& v){
    writeFuint64(v.size());
    write(v.c_str(), v.size());
}
void ByteArray::writeStringVint(const std::string& v){
    writeUint64(v.size());
    write(v.c_str(), v.size());
}
void ByteArray::writeStringWithoutLength(const std::string& v){
    write(v.c_str(), v.size());
}

int8_t   ByteArray::readFint8(){
    int8_t v;
    read(&v, sizeof(v));
    return v;
}
uint8_t  ByteArray::readFuint8(){
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}
int16_t  ByteArray::readFint16(){
    int16_t v;
    read(&v, sizeof(v));
    if(BYTE_ORDER == m_endian)
        return v;
    return byteswap(v);
}
uint16_t ByteArray::readFuint16(){
    uint16_t v;
    read(&v, sizeof(v));
    if(BYTE_ORDER == m_endian)
        return v;
    return byteswap(v);
}
int32_t  ByteArray::readFint32(){
    int32_t v;
    read(&v, sizeof(v));
    if(BYTE_ORDER == m_endian)
        return v;
    return byteswap(v);
}
uint32_t ByteArray::readFuint32(){
    uint32_t v;
    read(&v, sizeof(v));
    if(BYTE_ORDER == m_endian)
        return v;
    return byteswap(v);
}
int64_t  ByteArray::readFint64(){
    int64_t v;
    read(&v, sizeof(v));
    if(BYTE_ORDER == m_endian)
        return v;
    return byteswap(v);
}
uint64_t ByteArray::readFuint64(){
    uint64_t v;
    read(&v, sizeof(v));
    if(BYTE_ORDER == m_endian)
        return v;
    return byteswap(v);
}

int32_t  ByteArray::readInt32(){
    return DecodeZigzag32(readUint32());
}
uint32_t ByteArray::readUint32(){
    uint32_t result = 0;
    for(int i = 0; i < 32; i += 7){
        uint8_t b = readFuint8();
        if(b < 0x80){
            result |= ((uint32_t)b) << i;
            break;
        }
        result |= (((uint32_t)(b & 0x7f)) << i);
    }
    return result;
}
int64_t  ByteArray::readInt64(){
    return DecodeZigzag64(readUint64());
}
uint64_t ByteArray::readUint64(){
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7){
        uint8_t b = readFuint8();
        if(b < 0x80){
            result |= ((uint64_t)b) << i;
            break;
        }
        result |= (((uint64_t)(b & 0x7f)) << i);
    }
    return result;
}

float   ByteArray::readFloat(){
    uint32_t v = readFint32();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}
double  ByteArray::readDouble(){
    uint64_t v = readFint64();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

std::string ByteArray::readStringF16(){
    uint16_t len = readFuint16();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}
std::string ByteArray::readStringF32(){
    uint32_t len = readFuint32();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}
std::string ByteArray::readStringF64(){
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}
std::string ByteArray::readStringVint(){
    uint64_t len = readUint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

void ByteArray::clear(){
    m_position = m_size = 0;
    m_capacity = m_basesize;
    ByteArray::Node* tmp = m_root->next;
    while(tmp){
        m_current = tmp;
        tmp = tmp->next;
        delete tmp;
    }
    m_current = m_root;
    m_root->next = nullptr;
}

void ByteArray::write(const void* buf, size_t size){
    if(0 == size)
        return;
    addCapacity(size);
    size_t npos = m_position % m_basesize;
    size_t ncap = m_current->size - npos;
    size_t bpos = 0;

    while(0 < size){
        if(size <= ncap){
            memcpy(m_current->data + npos, (const char*)buf + bpos, size);
            if(m_current->size == npos + size)
                m_current = m_current->next;
            m_position += size;
            bpos += size;
            size = 0;
        }else{
            memcpy(m_current->data + npos, (const char*)buf + bpos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_current = m_current->next;
            ncap = m_current->size;
            npos = 0;
        }
    }

    if(m_size < m_position)
        m_size = m_position;
}
void ByteArray::read(void* buf, size_t size){
    if(getReadSize() < size)
        throw std::out_of_range("not enough len");
    
    size_t npos = m_position % m_basesize;
    size_t ncap = m_current->size - npos;
    size_t bpos = 0;

    while(0 < size){
        if(size <= ncap){
            memcpy((char*)buf + bpos, m_current->data + npos, size);
            if(m_current->size == npos + size)
                m_current = m_current->next;
            m_position += size;
            bpos += size;
            size = 0;
        }else{
            memcpy((char*)buf + bpos, m_current->data + npos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_current = m_current->next;
            ncap = m_current->size;
            npos = 0;
        }
    }
}

void ByteArray::read(void* buf, size_t size, size_t position) const{
    if(getReadSize() < size)
        throw std::out_of_range("not enough len");
    
    size_t npos = position % m_basesize;
    size_t ncap = m_current->size - npos;
    size_t bpos = 0;
    Node *cur = m_current;

    while(0 < size){
        if(size <= ncap){
            memcpy((char*)buf + bpos, cur->data + npos, size);
            if(cur->size == npos + size)
                cur = cur->next;
            position += size;
            bpos += size;
            size = 0;
        }else{
            memcpy((char*)buf + bpos, cur->data + npos, ncap);
            position += ncap;
            bpos += ncap;
            size -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}

void ByteArray::setPosition(size_t v){
    if(m_capacity < v)
        throw std::out_of_range("set_position out of range");
    m_position = v;
    if(m_size < m_position)
        m_size = m_position;
    m_current = m_root;
    while(m_current->size <= v){
        v -= m_current->size;
        m_current = m_current->next;
    }
    if(m_current->size == v)
        m_current = m_current->next;
}

bool ByteArray::writeToFile(const std::string& name) const{
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary);
    if(!ofs){
        LOG_ERROR(g_logger) << "writeToFile name=" << name << " error, errno="
            << errno << " errstr=" << strerror(errno);
            return false;
    }

    int64_t readSize = getReadSize();
    int64_t pos = m_position;
    Node* cur = m_current;

    while(0 < readSize){
        int diff = pos % m_basesize;
        int64_t len = ((int64_t)m_basesize < readSize ? m_basesize : readSize) - diff;
        ofs.write(cur->data + diff, len);
        cur = cur->next;
        pos += len;
        readSize -= len;
    }
    return true;
}
bool ByteArray::readFromFile(const std::string& name){
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    if(!ifs){
        LOG_ERROR(g_logger) << "readFromFile name=" << name << " error, errno="
            << errno << " errstr=" << strerror(errno);
            return false;
    }

    std::shared_ptr<char> buff(new char[m_basesize], std::default_delete<char[]>());
    while(!ifs.eof()){
        ifs.read(buff.get(), m_basesize);
        write(buff.get(), ifs.gcount());
    }
    return true;
}

bool ByteArray::isLittleEndian() const{
    return LITTLE_ENDIAN == m_endian;
}
void ByteArray::setIsLittleEndian(bool val){
    m_endian = val ? LITTLE_ENDIAN : BIG_ENDIAN;
}

int64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const{
    len = getReadSize() < len ? getReadSize() : len;
    if(0 == len) return 0;

    uint64_t size = len;
    size_t npos = m_position % m_basesize;
    size_t ncap = m_current->size - npos;
    struct iovec iov;
    Node *cur = m_current;

    while(0 < len){
        if(len <= ncap){
            iov.iov_base = cur->data + npos;
            iov.iov_len = len;
            len = 0;
        }else{
            iov.iov_base = cur->data + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}
int64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const{
    len = getReadSize() < len ? getReadSize() : len;
    if(0 == len) return 0;

    uint64_t size = len;
    size_t npos = position % m_basesize;
    size_t count = position / m_basesize;
    Node *cur = m_root;
    while(0 < count--) cur = cur->next;
    size_t ncap = cur->size - npos;
    struct iovec iov;

    while(0 < len){
        if(len <= ncap){
            iov.iov_base = cur->data + npos;
            iov.iov_len = len;
            len = 0;
        }else{
            iov.iov_base = cur->data + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}
int64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len){
    if(0 == len) return 0;
    addCapacity(len);
    uint64_t size = len;
    size_t npos = m_position % m_basesize;
    size_t ncap = m_current->size - npos;
    struct iovec iov;
    Node *cur = m_current;
    while(0 < len){
        if(len <= ncap){
            iov.iov_base = cur->data + npos;
            iov.iov_len = len;
            len = 0;
        }else{
            iov.iov_base = cur->data + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

void ByteArray::addCapacity(size_t size){
    if(0 == size) return;
    int oldCap = getCapacoty();
    if(size <= oldCap) return;
    size = size - oldCap;
    size_t count = (size / m_basesize) + (oldCap < size % m_basesize ? 1 : 0);
    Node* tmp = m_root;
    while(tmp->next)
        tmp = tmp->next;
    Node *first = nullptr;
    for(size_t i = 0; i < count; ++i){
        tmp->next = new Node(m_basesize);
        if(first == nullptr)
            first = tmp->next;
        tmp = tmp->next;
        m_capacity += m_basesize;
    }

    if(0 == oldCap)
        m_current = first;
}

std::string ByteArray::toString() const{
    std::string str;
    str.resize(getReadSize());
    if(str.empty()) return str;
    read(&str[0], str.size(), m_position);
    return str;
}
std::string ByteArray::toHexString(){
    std::string str = toString();
    std::stringstream ss;
    for(size_t i = 0; i < str.size(); ++i){
        if(i < 0 && 0 == i % 32)
            ss << std::endl;
        ss << std::setw(2) << std::setfill('0') << std::hex
            << (int)(uint8_t)str[i] << " ";
    }
    return ss.str();
}
__END__