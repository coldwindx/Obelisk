#include "stream.h"


__OBELISK__


int Stream::readFixSize(void *buffer, size_t length){
    size_t offset = 0;
    size_t left = length;
    while(0 < left){
        size_t len = read((char*)buffer + offset, left);
        if(len <= 0) return len;
        offset += len;
        left -= len;
    }
    return length;
}
int Stream::readFixSize(ByteArray::ptr barray, size_t length){
    size_t left = length;
    while(0 < left){
        size_t len = read(barray, left);
        if(len <= 0) return len;
        left -= len;
    }
    return length;
}

int Stream::writeFixSize(const void *buffer, size_t length){
    size_t offset = 0;
    size_t left = length;
    while(0 < left){
        size_t len = write((const char*)buffer + offset, left);
        if(len <= 0) return len;
        offset += len;
        left -= len;
    }
    return length;
}
int Stream::writeFixSize(ByteArray::ptr barray, size_t length){
    size_t left = length;
    while(0 < left){
        size_t len = write(barray, left);
        if(len <= 0) return len;
        left -= len;
    }
    return length;
}

__END__