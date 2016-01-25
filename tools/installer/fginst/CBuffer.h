#ifndef __CBUFFER_H__
#define __CBUFFER_H__

#define MAX_BUFFER_SIZE   8192

class CBuffer
{
public:
	CBuffer() : length(0), capacity(MAX_BUFFER_SIZE) {data = staticBuf;};
	~CBuffer() {if (data != staticBuf) free(data);};
	char* GetDataPtr() {return data;};
	int GetLength() {return length;};
	void SetLength(int len) {length = len; if (len < capacity) data[len] = '\0';};
	int GetCapacity() {return capacity;};
	void  Reserve(int newSize);

	CBuffer& operator= (const CBuffer& t); 

    operator char* () {return data; }; 

    void Append(const CBuffer& t);

    void Append(const char* newData, int dataLen);
protected:
	char* data;
	char  staticBuf[MAX_BUFFER_SIZE];
	int   length;
	int   capacity;
};

#endif 