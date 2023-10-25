#include "GameServer.h"

#include <process.h>
#include "NetLibrary/Logger/Logger.h"
#include "NetLibrary/Profiler/Profiler.h"

void GameServer::Start(const uint16_t port, const uint32_t maxSessionCount, const uint32_t iocpConcurrentThreadCount, const uint32_t iocpWorkerThreadCount)
{
#pragma region Map File Read
	FILE* fptr_map;
	ASSERT_LIVE(fopen_s(&fptr_map, "map.txt", "r") != EINVAL, L"map.txt open Failed");
	ASSERT_LIVE(fptr_map != nullptr, L"map.txt open Failed");

	while (true)
	{
		int x;
		int y;

		if (fscanf_s(fptr_map, "%d %d", &x, &y) != 2)
		{
			break;
		}

		mlPathFinder.Block(x, y);
	}

	LOGF(ELogLevel::System, L"Thread %d File Load Complete", GetCurrentThreadId());
#pragma endregion

    NetServer::Start(port, maxSessionCount, iocpConcurrentThreadCount, iocpWorkerThreadCount);

    HANDLE mUpdateThread = reinterpret_cast<HANDLE>(::_beginthreadex(nullptr, 0, updateThread, this, 0, nullptr));

	mUpdateThreadTimer = ::CreateWaitableTimer(NULL, FALSE, NULL);
	ASSERT_LIVE(mUpdateThreadTimer != INVALID_HANDLE_VALUE, L"mUpdateThreadTimer create failed");

	LARGE_INTEGER timerTime{};
	timerTime.QuadPart = -1 * (10'000 * static_cast<LONGLONG>(20));
	::SetWaitableTimer(mUpdateThreadTimer, &timerTime, 20, nullptr, nullptr, FALSE);
}

void GameServer::OnAccept(const uint64_t sessionID)
{
    std::lock_guard<std::mutex> lock(mGameLock);

    Player* newPlayer = mPlayerPool.Alloc();

    float x;
    float y;

    do
    {
        x = static_cast<float>(rand() % 99 + 1);
        y = static_cast<float>(rand() % 99 + 1);
    } while (mlPathFinder.IsBlocked(static_cast<int>(x), static_cast<int>(y)));

    newPlayer->Init(sessionID, ++mPlayerID, x, y);

    int32_t sectorX = getSectorX(x);
    int32_t sectorY = getSectorY(y);

    // 1. 접속한 클라이언트의 캐릭터 할당
    Serializer* SC_CREATE_MY_CHARACTER = Create_SC_CREATE_MY_CHARACTER(newPlayer->GetPlayerID(), newPlayer->GetX(), newPlayer->GetY());
    SendPacket(sessionID, SC_CREATE_MY_CHARACTER);
	Serializer::Free(SC_CREATE_MY_CHARACTER);

    // 2. 접속한 다른 클라이언트의 캐릭터 생성
    for (int i = sectorY - 1; i <= sectorY + 1; ++i)
    {
        for (int j = sectorX - 1; j <= sectorX + 1; ++j)
        {
            if (i < 0 || i >= RANGE_MOVE_BOTTOM / SECTOR_SIZE_Y || j < 0 || j >= RANGE_MOVE_RIGHT / SECTOR_SIZE_X)
            {
                continue;
            }

            for (auto otherPlayer : mSector[i][j])
            {
                {
                    Serializer* SC_CREATE_OTHER_CHARACTER = Create_SC_CREATE_OTHER_CHARACTER(otherPlayer->GetPlayerID(), otherPlayer->GetX(), otherPlayer->GetY());
                    SendPacket(sessionID, SC_CREATE_MY_CHARACTER);
					Serializer::Free(SC_CREATE_OTHER_CHARACTER);
                }

                if (otherPlayer->IsMoving())
                {
                    Serializer* SC_PATH_FIND = Create_SC_PATH_FIND(otherPlayer->GetPlayerID(), otherPlayer->GetDestPositions());
                    SendPacket(sessionID, SC_PATH_FIND);
					Serializer::Free(SC_PATH_FIND);
                }
            }
        }
    }

    // 3. 접속한 다른 클라이언트에게 방금 들어온 캐릭터를 생성하도록 함
    {
        Serializer* SC_CREATE_OTHER_CHARACTER = Create_SC_CREATE_OTHER_CHARACTER(newPlayer->GetPlayerID(), newPlayer->GetX(), newPlayer->GetY());
        sendSectorAround(SC_CREATE_OTHER_CHARACTER, newPlayer->GetPlayerID(), sectorX, sectorY);
		Serializer::Free(SC_CREATE_OTHER_CHARACTER);
    }

    mPlayerList.insert(std::make_pair(sessionID, newPlayer));
    addToSector(sectorX, sectorY, newPlayer);
}

void GameServer::OnReceive(const uint64_t sessionID, Serializer* packet)
{
    std::lock_guard<std::mutex> lock(mGameLock);

	uint8_t messageType;

	*packet >> messageType;

	switch (messageType)
	{
	case EMessageType::MESSAGE_TYPE_CS_MOVE:
	{
		float x;
		float y;

		*packet >> x >> y;

		process_CS_MOVE(sessionID, x, y);
	}
	break;
	case EMessageType::MESSAGE_TYPE_CS_HEARTBEAT:
	{
		if (packet->GetUseSize() != sizeof(messageType))
		{
			Disconnect(sessionID);
		}

		process_CS_HEARTBEAT(sessionID);
	}
	break;
	default:
		ASSERT_LIVE(false, L"Invalid Packet");
	}

	Serializer::Free(packet);
}

void GameServer::OnRelease(const uint64_t sessionID)
{
    std::lock_guard<std::mutex> lock(mGameLock);

	auto found = mPlayerList.find(sessionID);

	Player* player = found->second;

	int32_t sectorX = getSectorX(player->GetX());
	int32_t sectorY = getSectorY(player->GetY());

	mPlayerList.erase(sessionID);
	removeFromSector(sectorX, sectorY, player);

	Serializer* SC_DELETE_CHARACTER = Create_SC_DELETE_CHARACTER(player->GetPlayerID());
	sendSectorAround(SC_DELETE_CHARACTER, player->GetPlayerID(), sectorX, sectorY);

	mPlayerPool.Free(player);
}

void GameServer::process_CS_MOVE(const uint64_t sessionID, const float x, const float y)
{
	Player* player = mPlayerList.find(sessionID)->second;

	player->UpdateLastRecvTick();

	if (x < 0.0f || x >= 100.0f || y < 0.0f || y >= 100.0f)
	{
		return;
	}

	const int intX = static_cast<int>(x);
	const int intY = static_cast<int>(y);

	if (mlPathFinder.IsBlocked(intX, intY))
	{
		return;
	}

	player->UpdateLastTick();

	int startX;
	int startY;

	if (player->IsMoving())
	{
		startX = player->GetDestPositions().back().X;
		startY = player->GetDestPositions().back().Y;
		
	}
	else
	{
		startX = static_cast<int>(player->GetX());
		startY = static_cast<int>(player->GetY());
	}

	if (startX == intX && startY == intY)
	{
		return;
	}

	PROFILE_BEGIN(L"PathFind");
	auto it = mlPathFinder.PathFind(startX, startY, intX, intY);
	PROFILE_END(L"PathFind");

	++it;

	for (; it != mlPathFinder.End(); ++it)
	{
		player->PushToDestPositions(*it);
	}

	player->SetStateToMove();

	Serializer* SC_PATH_FIND = Create_SC_PATH_FIND(player->GetPlayerID(), mlPathFinder.GetPoints());
	sendSectorAround(SC_PATH_FIND, -1, getSectorX(player->GetX()), getSectorY(player->GetY()));
	Serializer::Free(SC_PATH_FIND);
}

void GameServer::process_CS_HEARTBEAT(const uint64_t sessionID)
{
	mPlayerList.find(sessionID)->second->UpdateLastRecvTick();
}

Serializer* GameServer::Create_SC_CREATE_MY_CHARACTER(const int32_t id, const float x, const float y)
{
    Serializer* packet = Serializer::Alloc();
    *packet << EMessageType::MESSAGE_TYPE_SC_CREATE_MY_CHARACTER << id << x << y;
    return packet;
}

Serializer* GameServer::Create_SC_CREATE_OTHER_CHARACTER(const int32_t id, const float x, const float y)
{
    Serializer* packet = Serializer::Alloc();
    *packet << EMessageType::MESSAGE_TYPE_SC_CREATE_OTHER_CHARACTER << id << x << y;
    return packet;
}

Serializer* GameServer::Create_SC_DELETE_CHARACTER(const int32_t id)
{
    Serializer* packet = Serializer::Alloc();
    *packet << EMessageType::MESSAGE_TYPE_SC_DELETE_CHARACTER << id;
    return packet;
}

Serializer* GameServer::Create_SC_MOVE(const int32_t id, const float x, const float y)
{
    Serializer* packet = Serializer::Alloc();
    *packet << EMessageType::MESSAGE_TYPE_SC_MOVE << id << x << y;
    return packet;
}

static Serializer& operator<<(Serializer& serializer, const std::list<Point>& points)
{
    for (auto it = points.begin(); it != points.end(); ++it)
    {
        serializer << (*it);
    }

    serializer << (int)points.size();

    return serializer;
}

Serializer* GameServer::Create_SC_PATH_FIND(const int32_t id, const std::list<Point>& points)
{
    Serializer* packet = Serializer::Alloc();
    *packet << EMessageType::MESSAGE_TYPE_SC_PATH_FIND << id << points;
    return packet;
}

unsigned int GameServer::updateThread(void* server)
{
	GameServer* gameServer = reinterpret_cast<GameServer*>(server);

	constexpr float SPEED = 4.0f;

	for (;;)
	{
		::WaitForSingleObject(gameServer->mUpdateThreadTimer, INFINITE);

		std::lock_guard<std::mutex> lock(gameServer->mGameLock);

		for (const auto& visit : gameServer->mPlayerList)
		{
			Player* player = visit.second;

			if (timeGetTime() - player->GetLastRecvTick() >= 40'000)
			{
				gameServer->Disconnect(visit.first);
				continue;
			}

			if (false == player->IsMoving())
			{
				continue;
			}

			DWORD tick = timeGetTime();
			float deltaTime = (tick - player->GetLastTick()) / 1000.0f;
			player->SetLastTick(tick);

			Point destination = player->GetDestPositions().front();

			float magnitude = std::sqrt((player->GetX() - destination.X) * (player->GetX() - destination.X)
				+ (player->GetY() - destination.Y) * (player->GetY() - destination.Y));

			if (magnitude < FLT_EPSILON)
			{
				player->PopDestPositions();

				if (player->GetDestPositions().size() == 0)
				{
					player->SetStateToIdle();
				}
			}
			else
			{
				float moveDistance = SPEED * deltaTime;

				if (moveDistance < 0.0f)
				{
					moveDistance = 0.0f;
				}
				else if (moveDistance > magnitude)
				{
					moveDistance = magnitude;
				}

				float newX = player->GetX() + (destination.X - player->GetX()) * moveDistance / magnitude;
				float newY = player->GetY() + (destination.Y - player->GetY()) * moveDistance / magnitude;

				gameServer->updateSector(player, static_cast<int>(newX), static_cast<int>(newY));

				player->Move(newX, newY);
			}
		}

		gameServer->mUpdateCount++;
	}

    return 0;
}

void GameServer::updateSector(Player* player, const int32_t newX, const int32_t newY)
{
    int32_t prevSectorX = getSectorX(player->GetX());
    int32_t prevSectorY = getSectorY(player->GetY());
    int32_t newSectorX = getSectorX(newX);
    int32_t newSectorY = getSectorY(newY);

    // 섹터가 변경되지 않는다면 리턴
    if (prevSectorX == newSectorX && prevSectorY == newSectorY)
    {
        return;
    }

    // 기존 섹터에서 캐릭터를 제거하고 새로운 섹터에 추가
    removeFromSector(prevSectorX, prevSectorY, player);
    addToSector(newSectorX, newSectorY, player);

    // 메세지 생성
    Serializer* SC_CREATE_OTHER_CHARACTER = Create_SC_CREATE_OTHER_CHARACTER(player->GetPlayerID(), static_cast<float>(newX), static_cast<float>(newY));
    Serializer* SC_DELETE_CHARACTER = Create_SC_DELETE_CHARACTER(player->GetPlayerID());
    Serializer* SC_PATH_FIND = Create_SC_PATH_FIND(player->GetPlayerID(), player->GetDestPositions());

    int sectorCount;		// 섹터 개수
    Point removeSectors[5]; // 기존 영향권에서 빠지는 섹터들
    Point addSectors[5];	// 새롭게 영향권에 추가되는 섹터들

	// 8방향에 대한 sectorCount, removeSectors, addSectors를 세팅
	if (prevSectorY == newSectorY && prevSectorX > newSectorX)
	{
		// ←
		sectorCount = 3;

		removeSectors[0] = { prevSectorX + 1, prevSectorY - 1 };
		removeSectors[1] = { prevSectorX + 1, prevSectorY };
		removeSectors[2] = { prevSectorX + 1, prevSectorY + 1 };

		addSectors[0] = { newSectorX - 1, prevSectorY - 1 };
		addSectors[1] = { newSectorX - 1, prevSectorY };
		addSectors[2] = { newSectorX - 1, prevSectorY + 1 };
	}
	else if (prevSectorY == newSectorY && prevSectorX < newSectorX)
	{
		// →
		sectorCount = 3;

		removeSectors[0] = { prevSectorX - 1, prevSectorY - 1 };
		removeSectors[1] = { prevSectorX - 1, prevSectorY };
		removeSectors[2] = { prevSectorX - 1, prevSectorY + 1 };

		addSectors[0] = { newSectorX + 1, prevSectorY - 1 };
		addSectors[1] = { newSectorX + 1, prevSectorY };
		addSectors[2] = { newSectorX + 1, prevSectorY + 1 };
	}
	else if (prevSectorY > newSectorY && prevSectorX == newSectorX)
	{
		// ↑
		sectorCount = 3;

		removeSectors[0] = { prevSectorX - 1, prevSectorY + 1 };
		removeSectors[1] = { prevSectorX, prevSectorY + 1 };
		removeSectors[2] = { prevSectorX + 1, prevSectorY + 1 };

		addSectors[0] = { newSectorX - 1, newSectorY - 1 };
		addSectors[1] = { newSectorX, newSectorY - 1 };
		addSectors[2] = { newSectorX + 1, newSectorY - 1 };
	}
	else if (prevSectorY < newSectorY && prevSectorX == newSectorX)
	{
		// ↓
		sectorCount = 3;

		removeSectors[0] = { prevSectorX - 1, prevSectorY - 1 };
		removeSectors[1] = { prevSectorX, prevSectorY - 1 };
		removeSectors[2] = { prevSectorX + 1, prevSectorY - 1 };

		addSectors[0] = { newSectorX - 1, newSectorY + 1 };
		addSectors[1] = { newSectorX, newSectorY + 1 };
		addSectors[2] = { newSectorX + 1, newSectorY + 1 };
	}
	else if (prevSectorY > newSectorY && prevSectorX > newSectorX)
	{
		// ↖
		sectorCount = 5;

		removeSectors[0] = { prevSectorX + 1, prevSectorY - 1 };
		removeSectors[1] = { prevSectorX + 1, prevSectorY };
		removeSectors[2] = { prevSectorX - 1, prevSectorY + 1 };
		removeSectors[3] = { prevSectorX, prevSectorY + 1 };
		removeSectors[4] = { prevSectorX + 1, prevSectorY + 1 };

		addSectors[0] = { newSectorX - 1, newSectorY - 1 };
		addSectors[1] = { newSectorX, newSectorY - 1 };
		addSectors[2] = { newSectorX + 1, newSectorY - 1 };
		addSectors[3] = { newSectorX - 1, newSectorY };
		addSectors[4] = { newSectorX - 1, newSectorY + 1 };

	}
	else if (prevSectorY > newSectorY && prevSectorX < newSectorX)
	{
		// ↗
		sectorCount = 5;

		removeSectors[0] = { prevSectorX - 1, prevSectorY - 1 };
		removeSectors[1] = { prevSectorX - 1, prevSectorY };
		removeSectors[2] = { prevSectorX - 1, prevSectorY + 1 };
		removeSectors[3] = { prevSectorX, prevSectorY + 1 };
		removeSectors[4] = { prevSectorX + 1, prevSectorY + 1 };

		addSectors[0] = { newSectorX - 1, newSectorY - 1 };
		addSectors[1] = { newSectorX, newSectorY - 1 };
		addSectors[2] = { newSectorX + 1, newSectorY - 1 };
		addSectors[3] = { newSectorX + 1, newSectorY };
		addSectors[4] = { newSectorX + 1, newSectorY + 1 };
	}
	else if (prevSectorY < newSectorY && prevSectorX > newSectorX)
	{
		// ↙
		sectorCount = 5;

		removeSectors[0] = { prevSectorX - 1, prevSectorY - 1 };
		removeSectors[1] = { prevSectorX, prevSectorY - 1 };
		removeSectors[2] = { prevSectorX + 1, prevSectorY - 1 };
		removeSectors[3] = { prevSectorX + 1, prevSectorY };
		removeSectors[4] = { prevSectorX + 1, prevSectorY + 1 };

		addSectors[0] = { newSectorX - 1, newSectorY - 1 };
		addSectors[1] = { newSectorX - 1, newSectorY };
		addSectors[2] = { newSectorX - 1, newSectorY + 1 };
		addSectors[3] = { newSectorX, newSectorY + 1 };
		addSectors[4] = { newSectorX + 1, newSectorY + 1 };
	}
	else if (prevSectorY < newSectorY && prevSectorX < newSectorX)
	{
		// ↘
		sectorCount = 5;

		removeSectors[0] = { prevSectorX - 1, prevSectorY - 1 };
		removeSectors[1] = { prevSectorX, prevSectorY - 1 };
		removeSectors[2] = { prevSectorX + 1, prevSectorY - 1 };
		removeSectors[3] = { prevSectorX - 1, prevSectorY };
		removeSectors[4] = { prevSectorX - 1, prevSectorY + 1 };

		addSectors[0] = { newSectorX + 1, newSectorY - 1 };
		addSectors[1] = { newSectorX + 1, newSectorY };
		addSectors[2] = { newSectorX - 1, newSectorY + 1 };
		addSectors[3] = { newSectorX, newSectorY + 1 };
		addSectors[4] = { newSectorX + 1, newSectorY + 1 };
	}

	// 삭제 정보 및 생성 정보 송신
	for (int i = 0; i < sectorCount; ++i)
	{
		// 1. 나에게 삭제 정보
		deleteSector(player, removeSectors[i].X, removeSectors[i].Y);

		// 2. 다른 클라이언트에게 나의 삭제 정보
		sendSector(SC_DELETE_CHARACTER, player->GetPlayerID(), removeSectors[i].X, removeSectors[i].Y);

		// 3. 나에게 생성 정보
		createSector(player, addSectors[i].X, addSectors[i].Y);

		// 4. 다른 클라이언트에게 나의 생성 정보
		sendSector(SC_CREATE_OTHER_CHARACTER, player->GetPlayerID(), addSectors[i].X, addSectors[i].Y);
		if (player->IsMoving())
		{
			sendSector(SC_PATH_FIND, player->GetPlayerID(), addSectors[i].X, addSectors[i].Y);
		}
	}

	// 메세지 반환
	Serializer::Free(SC_CREATE_OTHER_CHARACTER);
	Serializer::Free(SC_DELETE_CHARACTER);
	Serializer::Free(SC_PATH_FIND);
}

void GameServer::sendSector(Serializer* message, const int32_t exceptPlayerID, const int32_t sectorX, const int32_t sectorY)
{
	if (sectorX < 0 || sectorX >= RANGE_MOVE_RIGHT / SECTOR_SIZE_X)
	{
		return;
	}

	if (sectorY < 0 || sectorY >= RANGE_MOVE_BOTTOM / SECTOR_SIZE_Y)
	{
		return;
	}

	auto endIt = mSector[sectorY][sectorX].end();

	for (auto player : mSector[sectorY][sectorX])
	{
		if (player->GetPlayerID() == exceptPlayerID)
		{
			continue;
		}

		SendPacket(player->GetSessionID(), message);
	}
}

void GameServer::sendSectorAround(Serializer* message, const int32_t exceptPlayerID, const int32_t sectorX, const int32_t sectorY)
{
	sendSector(message, exceptPlayerID, sectorX - 1, sectorY - 1);
	sendSector(message, exceptPlayerID, sectorX - 1, sectorY);
	sendSector(message, exceptPlayerID, sectorX - 1, sectorY + 1);

	sendSector(message, exceptPlayerID, sectorX, sectorY - 1);
	sendSector(message, exceptPlayerID, sectorX, sectorY);
	sendSector(message, exceptPlayerID, sectorX, sectorY + 1);

	sendSector(message, exceptPlayerID, sectorX + 1, sectorY - 1);
	sendSector(message, exceptPlayerID, sectorX + 1, sectorY);
	sendSector(message, exceptPlayerID, sectorX + 1, sectorY + 1);
}

void GameServer::createSector(Player* player, const int32_t sectorX, const int32_t sectorY)
{
	if (sectorX < 0 || sectorX >= RANGE_MOVE_RIGHT / SECTOR_SIZE_X)
	{
		return;
	}

	if (sectorY < 0 || sectorY >= RANGE_MOVE_BOTTOM / SECTOR_SIZE_Y)
	{
		return;
	}

	auto endIt = mSector[sectorY][sectorX].end();

	for (auto visit : mSector[sectorY][sectorX])
	{
		if (visit->GetPlayerID() == player->GetPlayerID())
		{
			continue;
		}

		{
			Serializer* SC_CREATE_OTHER_CHARACTER = Create_SC_CREATE_OTHER_CHARACTER(visit->GetPlayerID(), visit->GetX(), visit->GetY());
			SendPacket(player->GetSessionID(), SC_CREATE_OTHER_CHARACTER);
			Serializer::Free(SC_CREATE_OTHER_CHARACTER);
		}

		if (player->IsMoving())
		{
			Serializer* SC_PATH_FIND = Create_SC_PATH_FIND(visit->GetPlayerID(), visit->GetDestPositions());
			SendPacket(player->GetSessionID(), SC_PATH_FIND);
			Serializer::Free(SC_PATH_FIND);
		}
	}
}

void GameServer::deleteSector(Player* player, const int32_t sectorX, const int32_t sectorY)
{
	if (sectorX < 0 || sectorX >= RANGE_MOVE_RIGHT / SECTOR_SIZE_X)
	{
		return;
	}

	if (sectorY < 0 || sectorY >= RANGE_MOVE_BOTTOM / SECTOR_SIZE_Y)
	{
		return;
	}

	auto endIt = mSector[sectorY][sectorX].end();

	for (auto visit : mSector[sectorY][sectorX])
	{
		if (visit->GetPlayerID() == player->GetPlayerID())
		{
			continue;
		}

		Serializer* SC_DELETE_CHARACTER = Create_SC_DELETE_CHARACTER(visit->GetPlayerID());
		SendPacket(player->GetSessionID(), SC_DELETE_CHARACTER);
		Serializer::Free(SC_DELETE_CHARACTER);
	}
}
