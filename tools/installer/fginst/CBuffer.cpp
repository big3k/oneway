#include "stdafx.h"
#include "CBuffer.h"

void  CBuffer::Reserve(int newSize) 
{
    if (newSize <= 0)
        return;

    if (newSize > capacity) {
        if (data == staticBuf)
        {
            data = (char*)malloc(newSize);
            memcpy(data, staticBuf, length);
        } else {
            data = (char*)realloc(data, newSize);
        }

        capacity = newSize;
    }
};

CBuffer& CBuffer::operator= (const CBuffer& t) 
{
	if (this == &t)
		return *this;

	//Free own buffer
	if (data != staticBuf) {
		free(data);
	}

	if (t.data == t.staticBuf) {
		data = staticBuf;
	} else {
		data = (char*)malloc(t.capacity);
	}

	memcpy(data, t.data, t.length);
    SetLength(t.length);
    capacity = t.capacity;

    return *this;
}

void CBuffer::Append(const CBuffer& t)
{
	if (t.length) {
		Reserve(length + t.length+1);
		memcpy(data+length, t.data, t.length);
		SetLength(length + t.length);
	}
}

void CBuffer::Append(const char* newData, int dataLen)
{
	if (dataLen) {
		Reserve(length + dataLen+1);
		memcpy(data+length, newData, dataLen);
		SetLength(length + dataLen);
	}
}

