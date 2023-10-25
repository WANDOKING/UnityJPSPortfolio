#pragma once

#include "NetLibrary/NetServer/NetServer.h"
#include "NetLibrary/NetServer/Serializer.h"
#include "MessageType.h"
#include "Player.h"
#include "AStarPathFinder.h"
#include "JPSPathFinder.h"

#include <Windows.h>
#include <list>
#include <unordered_map>
#include <mutex>

class GameServer : public NetServer
{
public:
    GameServer(void)
        : NetServer()
        , mlPathFinder(200, 200)
    {}

    virtual void Start(
        const uint16_t port,
        const uint32_t maxSessionCount,
        const uint32_t iocpConcurrentThreadCount,
        const uint32_t iocpWorkerThreadCount) override;

    inline uint32_t InitUpdateCount(void) { return mUpdateCount.exchange(0); }

private:

    // NetServer을(를) 통해 상속됨
    virtual void OnAccept(const uint64_t sessionID) override;
    virtual void OnReceive(const uint64_t sessionID, Serializer* packet) override;
    virtual void OnRelease(const uint64_t sessionID) override;

private: // 메세지 처리

    void process_CS_MOVE(const uint64_t sessionID, const float x, const float y);
    void process_CS_HEARTBEAT(const uint64_t sessionID);

private: // 메세지 생성

    static Serializer* Create_SC_CREATE_MY_CHARACTER(const int32_t id, const float x, const float y);
    static Serializer* Create_SC_CREATE_OTHER_CHARACTER(const int32_t id, const float x, const float y);
    static Serializer* Create_SC_DELETE_CHARACTER(const int32_t id);
    static Serializer* Create_SC_MOVE(const int32_t id, const float x, const float y);
    static Serializer* Create_SC_PATH_FIND(const int32_t id, const std::list<Point>& points);

private: // 스레드

    static unsigned int updateThread(void* server);

private:

    enum
    {
        RANGE_MOVE_BOTTOM = 100,
        RANGE_MOVE_RIGHT = 100,
        SECTOR_SIZE_X = 5,
        SECTOR_SIZE_Y = 5,
    };

    // 월드 좌표 X에 대한 섹터 X 인덱스를 얻는다
    inline static int32_t getSectorX(const int32_t worldX) { return worldX / SECTOR_SIZE_X; }
    inline static int32_t getSectorX(const float worldX) { return static_cast<int32_t>(worldX) / SECTOR_SIZE_X; }

    // 월드 좌표 Y에 대한 섹터 Y 인덱스를 얻는다
    inline static int32_t getSectorY(const int32_t worldY) { return worldY / SECTOR_SIZE_Y; }
    inline static int32_t getSectorY(const float worldX) { return static_cast<int32_t>(worldX) / SECTOR_SIZE_X; }

    inline void addToSector(const int32_t sectorX, const int32_t sectorY, Player* player)
    {
        mSector[sectorY][sectorX].push_back(player);
    }

    inline void removeFromSector(const int32_t sectorX, const int32_t sectorY, Player* player)
    {
        mSector[sectorY][sectorX].remove(player);
    }

    // player가 (newX, newY)로 이동했을 때, 섹터가 변경된다면 변경해준다
    void updateSector(Player* player, const int32_t newX, const int32_t newY);

    // 섹터에 대해 메세지 전파
    // 존재하지 않는 섹터면 보내지 않는다
    void sendSector(Serializer* message, const int32_t exceptPlayerID, const int32_t sectorX, const int32_t sectorY);

    // 현재 내 섹터 + 주위 섹터 총 9개의 섹터에 SendSector()를 호출
    void sendSectorAround(Serializer* message, const int32_t exceptPlayerID, const int32_t sectorX, const int32_t sectorY);

    // player 에게 해당 섹터의 플레이어들을 생성하라는 메세지를 보낸다
    // 존재하지 않는 섹터면 보내지 않는다
    void createSector(Player* player, const int32_t sectorX, const int32_t sectorY);

    // player 에게 해당 섹터의 플레이어들을 삭제하라는 메세지를 보낸다
    // 존재하지 않는 섹터면 보내지 않는다
    void deleteSector(Player* player, const int32_t sectorX, const int32_t sectorY);


private:
    int32_t     mPlayerID = 0;
    HANDLE      mUpdateThread = 0;
    HANDLE      mUpdateThreadTimer = 0;
    std::mutex  mGameLock;

    std::unordered_map<uint64_t, Player*>   mPlayerList;
    std::list<Player*>                      mSector[RANGE_MOVE_BOTTOM / SECTOR_SIZE_Y][RANGE_MOVE_RIGHT / SECTOR_SIZE_X];
    inline static OBJECT_POOL<Player>       mPlayerPool;
    JPSPathFinder                           mlPathFinder;

    std::atomic<uint32_t> mUpdateCount = 0;
};