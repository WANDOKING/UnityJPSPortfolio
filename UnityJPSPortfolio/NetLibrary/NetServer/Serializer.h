#pragma once

#include <cstdint>
#include <cstring>
#include <random>

#include "../CrashDump/CrashDump.h"
#include "NetworkHeader.h"
#include "../Memory/TlsObjectPool.h"

// 직렬화 버퍼 클래스
class Serializer
{
    friend class TlsObjectPool<Serializer>;
    friend class NetServer;
    friend class NetClient;

public:
    inline static Serializer* Alloc(void)
    {
        Serializer* ret = l_packetPool.Alloc();
        ret->Clear();
        ret->IncrementRefCount();
        return ret;
    };

    inline static void Free(Serializer* packet) { packet->DecrementRefCount(); }

    // 패킷 풀이 할당한 총 패킷의 개수
    inline static uint32_t GetTotalPacketCount(void) { return l_packetPool.GetTotalCreatedObjectCount(); }

public:

    inline bool     IsSendPrepared(void) const { return mbSendPrepared; }
    inline uint32_t	GetCapacity(void) const { return mCapacity; }
    inline uint32_t	GetFullSize(void) const { return mSize + sizeof(NetworkHeader); }
    inline uint32_t	GetUseSize(void) const { return mSize; }
    inline uint32_t	GetFreeSize(void) const { return mCapacity - mSize; }

    inline void	Clear(void)
    {
        mbSendPrepared = false;
        mSize = 0;
        mDeserialingSize = 0;
    }

    inline void	SetUseSize(const uint32_t size) { mSize = size; }

    // 참조 카운트를 1 증가 시킨다
    inline void	IncrementRefCount(void)
    {
        InterlockedIncrement(&mRefCount);
    }

    // 참조 카운트를 1 감소 시킨다 (0이 된다면 할당 해제)
    inline void	DecrementRefCount(void)
    {
        if (InterlockedDecrement(&mRefCount) == 0)
        {
            l_packetPool.Free(this);
        }
    }

    // 할당한 버퍼의 시작 포인터 (네트워크 헤더의 시작)
    inline char* GetFullBufferPointer(void) const { return mBuffer - sizeof(NetworkHeader); }

    // 버퍼의 시작 포인터 (컨텐츠 헤더의 시작)
    inline char* GetUserBufferPointer(void) const { return mBuffer; }

public:

    // 메모리를 그대로 복사해서 집어넣음 (UseSize도 늘어남)
    inline bool InsertByte(const char* source, const uint32_t size)
    {
        if (GetFreeSize() < size)
        {
            return false;
        }

        memcpy(mBuffer + mSize, source, size);
        mSize += size;

        return true;
    }

    // 현재 위치에서 메모리를 그대로 복사해옴 (DeserialingSize도 늘어남)
    inline bool GetByte(char* dest, const uint32_t size)
    {
        if (mDeserialingSize + size > mSize)
        {
            return false;
        }

        memcpy(dest, mBuffer + mDeserialingSize, size);
        mDeserialingSize += size;

        return true;
    }

#pragma region operator overloading - serializing
    inline Serializer& operator<<(unsigned char value)
    {
        CrashDump::Assert(GetFreeSize() >= sizeof(value));

        *(unsigned char*)(mBuffer + mSize) = value;
        mSize += sizeof(unsigned char);

        return *this;
    }

    inline Serializer& operator<<(char value)
    {
        CrashDump::Assert(GetFreeSize() >= sizeof(value));

        *(char*)(mBuffer + mSize) = value;
        mSize += sizeof(char);

        return *this;
    }

    inline Serializer& operator<<(unsigned short value)
    {
        CrashDump::Assert(GetFreeSize() >= sizeof(value));

        *(unsigned short*)(mBuffer + mSize) = value;
        mSize += sizeof(unsigned short);

        return *this;
    }

    inline Serializer& operator<<(short value)
    {
        CrashDump::Assert(GetFreeSize() >= sizeof(value));

        *(short*)(mBuffer + mSize) = value;
        mSize += sizeof(short);

        return *this;
    }

    inline Serializer& operator<<(unsigned int value)
    {
        CrashDump::Assert(GetFreeSize() >= sizeof(value));

        *(unsigned int*)(mBuffer + mSize) = value;
        mSize += sizeof(unsigned int);

        return *this;
    }

    inline Serializer& operator<<(int value)
    {
        CrashDump::Assert(GetFreeSize() >= sizeof(value));

        *(int*)(mBuffer + mSize) = value;
        mSize += sizeof(int);

        return *this;
    }

    inline Serializer& operator<<(unsigned long value)
    {
        CrashDump::Assert(GetFreeSize() >= sizeof(value));

        *(unsigned long*)(mBuffer + mSize) = value;
        mSize += sizeof(unsigned long);

        return *this;
    }

    inline Serializer& operator<<(long value)
    {
        CrashDump::Assert(GetFreeSize() >= sizeof(value));

        *(long*)(mBuffer + mSize) = value;
        mSize += sizeof(long);

        return *this;
    }

    inline Serializer& operator<<(unsigned long long value)
    {
        CrashDump::Assert(GetFreeSize() >= sizeof(value));

        *(unsigned long long*)(mBuffer + mSize) = value;
        mSize += sizeof(unsigned long long);

        return *this;
    }

    inline Serializer& operator<<(long long value)
    {
        CrashDump::Assert(GetFreeSize() >= sizeof(value));

        *(long long*)(mBuffer + mSize) = value;
        mSize += sizeof(long long);

        return *this;
    }

    inline Serializer& operator<<(float value)
    {
        CrashDump::Assert(GetFreeSize() >= sizeof(value));

        *(float*)(mBuffer + mSize) = value;
        mSize += sizeof(float);

        return *this;
    }

    inline Serializer& operator<<(double value)
    {
        CrashDump::Assert(GetFreeSize() >= sizeof(value));

        *(double*)(mBuffer + mSize) = value;
        mSize += sizeof(double);

        return *this;
    }
#pragma endregion

#pragma region operator overloading - deserializing
    inline Serializer& operator>>(unsigned char& value)
    {
        CrashDump::Assert(mDeserialingSize + sizeof(unsigned char) <= mSize);

        value = *(unsigned char*)(mBuffer + mDeserialingSize);
        mDeserialingSize += sizeof(unsigned char);

        return *this;
    }

    inline Serializer& operator>>(char& value)
    {
        CrashDump::Assert(mDeserialingSize + sizeof(char) <= mSize);

        value = *(char*)(mBuffer + mDeserialingSize);
        mDeserialingSize += sizeof(char);

        return *this;
    }

    inline Serializer& operator>>(unsigned short& value)
    {
        CrashDump::Assert(mDeserialingSize + sizeof(unsigned short) <= mSize);

        value = *(unsigned short*)(mBuffer + mDeserialingSize);
        mDeserialingSize += sizeof(unsigned short);

        return *this;
    }

    inline Serializer& operator>>(short& value)
    {
        CrashDump::Assert(mDeserialingSize + sizeof(short) <= mSize);

        value = *(short*)(mBuffer + mDeserialingSize);
        mDeserialingSize += sizeof(short);

        return *this;
    }

    inline Serializer& operator>>(unsigned int& value)
    {
        CrashDump::Assert(mDeserialingSize + sizeof(unsigned int) <= mSize);

        value = *(unsigned int*)(mBuffer + mDeserialingSize);
        mDeserialingSize += sizeof(unsigned int);

        return *this;
    }

    inline Serializer& operator>>(int& value)
    {
        CrashDump::Assert(mDeserialingSize + sizeof(int) <= mSize);

        value = *(int*)(mBuffer + mDeserialingSize);
        mDeserialingSize += sizeof(int);

        return *this;
    }

    inline Serializer& operator>>(unsigned long& value)
    {
        CrashDump::Assert(mDeserialingSize + sizeof(unsigned long) <= mSize);

        value = *(unsigned long*)(mBuffer + mDeserialingSize);
        mDeserialingSize += sizeof(unsigned long);

        return *this;
    }

    inline Serializer& operator>>(long& value)
    {
        CrashDump::Assert(mDeserialingSize + sizeof(long) <= mSize);

        value = *(long*)(mBuffer + mDeserialingSize);
        mDeserialingSize += sizeof(long);

        return *this;
    }

    inline Serializer& operator>>(unsigned long long& value)
    {
        CrashDump::Assert(mDeserialingSize + sizeof(unsigned long long) <= mSize);

        value = *(unsigned long long*)(mBuffer + mDeserialingSize);
        mDeserialingSize += sizeof(unsigned long long);

        return *this;
    }

    inline Serializer& operator>>(long long& value)
    {
        CrashDump::Assert(mDeserialingSize + sizeof(long long) <= mSize);

        value = *(long long*)(mBuffer + mDeserialingSize);
        mDeserialingSize += sizeof(long long);

        return *this;
    }

    inline Serializer& operator>>(float& value)
    {
        CrashDump::Assert(mDeserialingSize + sizeof(float) <= mSize);

        value = *(float*)(mBuffer + mDeserialingSize);
        mDeserialingSize += sizeof(float);

        return *this;
    }

    inline Serializer& operator>>(double& value)
    {
        CrashDump::Assert(mDeserialingSize + sizeof(double) <= mSize);

        value = *(double*)(mBuffer + mDeserialingSize);
        mDeserialingSize += sizeof(double);

        return *this;
    }
#pragma endregion
private:
    enum
    {
        DEFAULT_SIZE = 1'000
    };

    inline Serializer()
        : mCapacity(DEFAULT_SIZE)
    {
        mBuffer = (new char[DEFAULT_SIZE + sizeof(NetworkHeader)] + sizeof(NetworkHeader));
    }

    inline ~Serializer()
    {
        delete[](mBuffer - sizeof(NetworkHeader));
    }

    // 헤더에 값을 채운다
    void setHeaderValue(void)
    {
        NetworkHeader* header = reinterpret_cast<NetworkHeader*>(GetFullBufferPointer());

#if NETWORK_HEADER_USE_TYPE == NETWORK_HEADER_TYPE_LAN

        header->Length = static_cast<uint16_t>(GetUseSize());

#elif NETWORK_HEADER_USE_TYPE == NETWORK_HEADER_TYPE_NET

        header->Code = NETWORK_HEADER_CODE;
        header->Length = static_cast<uint8_t>(GetUseSize());

#endif
    }

    // 헤더를 세팅하고 인코딩을 수행 - SendQueue에 값을 넣기 전 이 함수를 호출해야 함
    void prepareSend(void)
    {
        mbSendPrepared = true;
        setHeaderValue();
    }

private:
    char* mBuffer;
    bool mbSendPrepared = false;
    uint32_t mCapacity;
    uint32_t mRefCount = 0;
    uint32_t mSize = 0;
    uint32_t mDeserialingSize = 0;

    // 패킷 풀
    inline static thread_local TlsObjectPool<Serializer> l_packetPool;
};