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

    // NetServer��(��) ���� ��ӵ�
    virtual void OnAccept(const uint64_t sessionID) override;
    virtual void OnReceive(const uint64_t sessionID, Serializer* packet) override;
    virtual void OnRelease(const uint64_t sessionID) override;

private: // �޼��� ó��

    void process_CS_MOVE(const uint64_t sessionID, const float x, const float y);
    void process_CS_HEARTBEAT(const uint64_t sessionID);

private: // �޼��� ����

    static Serializer* Create_SC_CREATE_MY_CHARACTER(const int32_t id, const float x, const float y);
    static Serializer* Create_SC_CREATE_OTHER_CHARACTER(const int32_t id, const float x, const float y);
    static Serializer* Create_SC_DELETE_CHARACTER(const int32_t id);
    static Serializer* Create_SC_MOVE(const int32_t id, const float x, const float y);
    static Serializer* Create_SC_PATH_FIND(const int32_t id, const std::list<Point>& points);

private: // ������

    static unsigned int updateThread(void* server);

private:

    enum
    {
        RANGE_MOVE_BOTTOM = 100,
        RANGE_MOVE_RIGHT = 100,
        SECTOR_SIZE_X = 5,
        SECTOR_SIZE_Y = 5,
    };

    // ���� ��ǥ X�� ���� ���� X �ε����� ��´�
    inline static int32_t getSectorX(const int32_t worldX) { return worldX / SECTOR_SIZE_X; }
    inline static int32_t getSectorX(const float worldX) { return static_cast<int32_t>(worldX) / SECTOR_SIZE_X; }

    // ���� ��ǥ Y�� ���� ���� Y �ε����� ��´�
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

    // player�� (newX, newY)�� �̵����� ��, ���Ͱ� ����ȴٸ� �������ش�
    void updateSector(Player* player, const int32_t newX, const int32_t newY);

    // ���Ϳ� ���� �޼��� ����
    // �������� �ʴ� ���͸� ������ �ʴ´�
    void sendSector(Serializer* message, const int32_t exceptPlayerID, const int32_t sectorX, const int32_t sectorY);

    // ���� �� ���� + ���� ���� �� 9���� ���Ϳ� SendSector()�� ȣ��
    void sendSectorAround(Serializer* message, const int32_t exceptPlayerID, const int32_t sectorX, const int32_t sectorY);

    // player ���� �ش� ������ �÷��̾���� �����϶�� �޼����� ������
    // �������� �ʴ� ���͸� ������ �ʴ´�
    void createSector(Player* player, const int32_t sectorX, const int32_t sectorY);

    // player ���� �ش� ������ �÷��̾���� �����϶�� �޼����� ������
    // �������� �ʴ� ���͸� ������ �ʴ´�
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