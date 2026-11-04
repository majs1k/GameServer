// -------------------------------------------------------------------
//
// 직렬화 버퍼
//
// 헤더와 바디 모두 포함
// 기본자료형 이외에는 <<, >> 전역 오버로딩 연산
// recv() 이후 생성된 직렬화버퍼에서 >> 사용시 에러 확인
// 
// -------------------------------------------------------------------
#pragma once
#include <atomic>
#include "MemoryPoolTls.h"
#include "Random.h"

using sbyte = int8_t;
using byte = uint8_t;
using ushort = uint16_t;
using uint = uint32_t;


#pragma pack(push, 1)
// ------------------------------------------------------- //


inline constexpr static unsigned char NET_CODE = 0x77;
inline constexpr static unsigned char NET_KEY = 0x32;

struct NET_HEADER
{
	unsigned char code_;
	short size_;
	unsigned char randKey_;
	// 암호화 대상
	unsigned char checksum_;
};

// ------------------------------------------------------- //

struct LAN_HEADER
{
	short size_;
};

// ------------------------------------------------------- //

#define PACKET_CODE				0x89

struct FIGHTER_HEADER
{
	unsigned char code_;
	char size_;
	char type_;
};

// ------------------------------------------------------- //

struct MY_HEADER
{
	short size_;
	short type_;
};

// ------------------------------------------------------- //

#pragma pack(pop)

// TODO: 용량 수정
#define PACKET_CAPACITY				2000
#define HEADER_CAPACITY				5

class Packet;

inline MemoryPoolTls<Packet> packetPool_;

class Packet
{
private:

	char* buffer_ = nullptr;

	int	capacity_ = PACKET_CAPACITY;

	int writePos_ = 0;
	int readPos_ = 0;

	int headerSize_ = 0;

	bool isError_ = false;
	bool isEncoding_ = false;
	std::atomic<int> refCount_ = 0;


public:

	Packet()
	{
		buffer_ = new char[capacity_];
	}

	~Packet()
	{
		delete[] buffer_;
	}

	void Initialize(int headerSize)
	{
		writePos_ = HEADER_CAPACITY;
		readPos_ = HEADER_CAPACITY;

		headerSize_ = headerSize;
		isError_ = false;
		isEncoding_ = false;
		refCount_ = 1;
	}

	void Clear(int headerSize)
	{
		writePos_ = HEADER_CAPACITY - headerSize;
		readPos_ = HEADER_CAPACITY - headerSize;

		headerSize_ = headerSize;
		isError_ = false;
		isEncoding_ = false;
		refCount_ = 1;
	}

	void AddRef()
	{
		++refCount_;
	}

	void Free()
	{
		if (--refCount_ == 0)
		{
			packetPool_.Free(this);
		}
	}

	char* GetHeaderPtr() const
	{
		return buffer_ + HEADER_CAPACITY - headerSize_;
	}

	char* GetBodyPtr() const
	{
		return buffer_ + HEADER_CAPACITY;
	}

	char* GetFrontBufferPtr() const
	{
		return buffer_ + readPos_;
	}

	char* GetRearBufferPtr() const
	{
		return buffer_ + writePos_;
	}

	int GetHeaderSize() const
	{
		return headerSize_;
	}

	// 헤더 제외
	int	UseSize() const
	{
		return writePos_ - readPos_;
	}

	int	FreeSize() const
	{
		return capacity_ - writePos_;
	}

	void SetLanHeader()
	{
		LAN_HEADER* header = reinterpret_cast<LAN_HEADER*>(GetHeaderPtr());

		header->size_ = UseSize();
	}

	void SetNetHeader()
	{
		NET_HEADER* header = reinterpret_cast<NET_HEADER*>(GetHeaderPtr());

		header->code_ = NET_CODE;
		header->size_ = UseSize();
		header->randKey_ = RAND(0, 255);

		int sum = 0;

		for (int i = 0; i < UseSize(); i++)
		{
			sum += *(reinterpret_cast<unsigned char*>(GetBodyPtr() + i));
		}

		header->checksum_ = sum % 256;
	}

	bool IsValidChecksum() const
	{
		NET_HEADER* header = reinterpret_cast<NET_HEADER*>(GetHeaderPtr());

		int sum = 0;

		unsigned char checksum = 0;

		for (int i = 0; i < UseSize(); i++)
		{
			sum += *(reinterpret_cast<unsigned char*>(GetBodyPtr() + i));
		}

		checksum = sum % 256;

		if (header->checksum_ != checksum)
			return false;

		return true;
	}

	void Encoding()
	{
		NET_HEADER* header = reinterpret_cast<NET_HEADER*>(GetHeaderPtr());
		unsigned char randKey = header->randKey_;

		/// 체크섬부터 인코딩
		char* buffer = GetBodyPtr() - 1;

		int s = 0;

		unsigned char p;
		unsigned char e;

		p = buffer[s] ^ (randKey + 1);
		e = p ^ (NET_KEY + 1);
		buffer[0] = e;

		while (UseSize() + 1 > ++s)
		{
			p = buffer[s] ^ (p + randKey + s + 1);
			e = p ^ (e + NET_KEY + s + 1);

			buffer[s] = e;
		}

		isEncoding_ = true;
	}

	void Decoding()
	{
		NET_HEADER* header = reinterpret_cast<NET_HEADER*>(GetHeaderPtr());
		unsigned char randKey = header->randKey_;

		/// 체크섬부터 디코딩
		char* buffer = GetBodyPtr() - 1;

		int s = 0;

		unsigned char p;
		unsigned char d;

		unsigned char oldE;
		unsigned char oldP;

		p = buffer[s] ^ (NET_KEY + 1);
		d = p ^ (randKey + 1);

		oldE = buffer[s];
		oldP = p;

		buffer[0] = d;

		while (UseSize() + 1 > ++s)
		{
			p = buffer[s] ^ (oldE + NET_KEY + s + 1);
			d = p ^ (oldP + randKey + s + 1);

			oldE = buffer[s];
			oldP = p;

			buffer[s] = d;
		}
	}

	bool IsEncoding() const
	{
		return isEncoding_;
	}

	bool IsError() const
	{
		return isError_;
	}

	int	MoveWritePos(int size)
	{
		writePos_ += size;
		return size;
	}

	int	MoveReadPos(int size)
	{
		readPos_ += size;
		return size;
	}

	Packet& operator << (char value)
	{
		*reinterpret_cast<unsigned char*>(buffer_ + writePos_) = value;
		writePos_ += sizeof(char);

		return *this;
	}

	Packet& operator << (unsigned char value)
	{
		*reinterpret_cast<unsigned char*>(buffer_ + writePos_) = value;
		writePos_ += sizeof(unsigned char);

		return *this;
	}

	Packet& operator << (short value)
	{
		*reinterpret_cast<short*>(buffer_ + writePos_) = value;
		writePos_ += sizeof(short);

		return *this;
	}

	Packet& operator << (unsigned short value)
	{
		*reinterpret_cast<unsigned short*>(buffer_ + writePos_) = value;
		writePos_ += sizeof(unsigned short);

		return *this;
	}

	Packet& operator << (int value)
	{
		*reinterpret_cast<int*>(buffer_ + writePos_) = value;
		writePos_ += sizeof(int);

		return *this;
	}

	Packet& operator << (unsigned int value)
	{
		*reinterpret_cast<unsigned int*>(buffer_ + writePos_) = value;
		writePos_ += sizeof(unsigned int);

		return *this;
	}

	Packet& operator << (float value)
	{
		*reinterpret_cast<float*>(buffer_ + writePos_) = value;
		writePos_ += sizeof(float);

		return *this;
	}

	Packet& operator << (int64_t value)
	{
		*reinterpret_cast<int64_t*>(buffer_ + writePos_) = value;
		writePos_ += sizeof(int64_t);

		return *this;
	}

	Packet& operator << (uint64_t value)
	{
		*reinterpret_cast<uint64_t*>(buffer_ + writePos_) = value;
		writePos_ += sizeof(uint64_t);

		return *this;
	}

	Packet& operator << (double value)
	{
		*reinterpret_cast<double*>(buffer_ + writePos_) = value;
		writePos_ += sizeof(double);

		return *this;
	}



	Packet& operator >> (char& value)
	{
		if (readPos_ + sizeof(char) > writePos_)
		{
			isError_ = true;

			return  *this;
		}

		value = *reinterpret_cast<char*>(buffer_ + readPos_);
		readPos_ += sizeof(char);

		return *this;
	}

	Packet& operator >> (unsigned char& value)
	{
		if (readPos_ + sizeof(unsigned char) > writePos_)
		{
			isError_ = true;

			return  *this;
		}

		value = *reinterpret_cast<unsigned char*>(buffer_ + readPos_);
		readPos_ += sizeof(unsigned char);

		return *this;
	}

	Packet& operator >> (short& value)
	{
		if (readPos_ + sizeof(short) > writePos_)
		{
			isError_ = true;

			return  *this;
		}

		value = *reinterpret_cast<short*>(buffer_ + readPos_);
		readPos_ += sizeof(short);

		return *this;
	}

	Packet& operator >> (unsigned short& value)
	{
		if (readPos_ + sizeof(unsigned short) > writePos_)
		{
			isError_ = true;

			return  *this;
		}

		value = *reinterpret_cast<unsigned short*>(buffer_ + readPos_);
		readPos_ += sizeof(unsigned short);

		return *this;
	}

	Packet& operator >> (int& value)
	{
		if (readPos_ + sizeof(int) > writePos_)
		{
			isError_ = true;

			return  *this;
		}

		value = *reinterpret_cast<int*>(buffer_ + readPos_);
		readPos_ += sizeof(int);

		return *this;
	}

	Packet& operator >> (unsigned int& value)
	{
		if (readPos_ + sizeof(unsigned int) > writePos_)
		{
			isError_ = true;

			return  *this;
		}

		value = *reinterpret_cast<unsigned int*>(buffer_ + readPos_);
		readPos_ += sizeof(unsigned int);

		return *this;
	}

	Packet& operator >> (float& value)
	{
		if (readPos_ + sizeof(float) > writePos_)
		{
			isError_ = true;

			return  *this;
		}

		value = *reinterpret_cast<float*>(buffer_ + readPos_);
		readPos_ += sizeof(float);

		return *this;
	}

	Packet& operator >> (int64_t& value)
	{
		if (readPos_ + sizeof(int64_t) > writePos_)
		{
			isError_ = true;

			return  *this;
		}

		value = *reinterpret_cast<int64_t*>(buffer_ + readPos_);
		readPos_ += sizeof(int64_t);

		return *this;
	}

	Packet& operator >> (uint64_t& value)
	{
		if (readPos_ + sizeof(uint64_t) > writePos_)
		{
			isError_ = true;

			return  *this;
		}

		value = *reinterpret_cast<uint64_t*>(buffer_ + readPos_);
		readPos_ += sizeof(uint64_t);

		return *this;
	}

	Packet& operator >> (double& value)
	{
		if (readPos_ + sizeof(double) > writePos_)
		{
			isError_ = true;

			return  *this;
		}

		value = *reinterpret_cast<double*>(buffer_ + readPos_);
		readPos_ += sizeof(double);

		return *this;
	}

	int	Write(const char* src, int size)
	{
		memcpy(buffer_ + writePos_, src, size);
		writePos_ += size;

		return size;
	}

	int	Read(char* dest, int size)
	{
		if (size <= 0 || readPos_ + size > writePos_)
		{
			isError_ = true;

			return 0;
		}

		memcpy(dest, buffer_ + readPos_, size);
		readPos_ += size;

		return size;
	}
};
