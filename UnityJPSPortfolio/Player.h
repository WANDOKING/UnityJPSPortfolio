#pragma once

#include <list>

#include "Point.h"

class Player
{
public:

    void Init(const uint64_t sessionID, const int32_t playerID, const float x, const float y)
    {
        mSessionID = sessionID;
        mPlayerID = playerID;
        mX = x;
        mY = y;

        mName.clear();
        mDestPositions.clear();
        mState = EPlayerState::Idle;
        mLastRecvTick = ::timeGetTime();
        mLastTick = 0;
    }

    inline uint64_t GetSessionID(void) const { return mSessionID; }
    inline int32_t  GetPlayerID(void) const { return mPlayerID; }
    inline float    GetX(void) const { return mX; }
    inline float    GetY(void) const { return mY; }

    inline bool     IsMoving(void) const { return mState == EPlayerState::Moving; }

    inline void     SetStateToMove(void) { mState = EPlayerState::Moving; }
    inline void     SetStateToIdle(void) { mState = EPlayerState::Idle; }

    inline const std::list<Point>& GetDestPositions(void) const { return mDestPositions; }

    inline void PushToDestPositions(const Point& point) { mDestPositions.push_back(point); }
    inline void PopDestPositions(void) { mDestPositions.pop_front(); }

    inline uint32_t GetLastTick(void) const { return mLastTick; }


    inline uint32_t GetLastRecvTick(void) const { return mLastRecvTick; }

    // LastRecvTick 갱신, 이전 값을 반환
    inline uint32_t UpdateLastRecvTick(void)
    {
        uint32_t ret = mLastRecvTick;
        mLastRecvTick = ::timeGetTime();
        return ret;
    }

    // LastTick 갱신, 이전 값을 반환
    inline uint32_t UpdateLastTick(void)
    {
        uint32_t ret = mLastTick;
        mLastTick = ::timeGetTime(); 
        return ret;
    }

    inline void SetLastTick(uint32_t tick) { mLastTick = tick; }

    inline void Move(float x, float y)
    {
        mX = x;
        mY = y;

        if (mX < 0.0f)
        {
            mX = 0.0f;
        }

        if (mY < 0.0f)
        {
            mY = 0.0f;
        }
    }

    enum EPlayerState
    {
        Moving,
        Idle,
    };

private:
    uint64_t            mSessionID;
    int32_t             mPlayerID;
    float               mX;
    float               mY;
    std::wstring        mName;
    std::list<Point>    mDestPositions;
    EPlayerState        mState;
    uint32_t            mLastRecvTick;   // timeout을 위한 tick
    uint32_t            mLastTick;       // 프레임마다 이동을 위한 tick
};