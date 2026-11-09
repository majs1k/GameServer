#include "ChatServer.h"
#include "RpcModule.h"


void ChatServer::ProcessMessageQueue()
{
	while (1)
	{
		Message* message;

		/// 해당 맵의 메세지 처리
		bool ret = messageQueue_[tls_MapId_].Dequeue(message);

		if (ret == false)
			break;

		procMessageCount_++;

		uint64_t sessionId = message->sessionId_;
		Packet* packet = message->packet_;
		ushort protocol = message->protocol_;
		
		Player* player = message->player_;


		switch (protocol)
		{
		case CS_NETWORK_PACKET:
		{
			/// 네트워크 패킷 처리 (타입 자료형 주의)
			if (!PacketProc(sessionId, packet))
			{
				Disconnect(sessionId);
			}

			messageCount_CS_NETWORK_PACKET_++;

			break;
		}

		case SS_CREATE_PLAYER:

			SS_CreatePlayer(sessionId);

			messageCount_SS_CreatePlayer_++;

			break;

		case SS_REMOVE_PLAYER:

			SS_RemovePlayer(sessionId);

			messageCount_SS_RemovePlayer_++;

			break;

		case SS_CHECK_TIMEOUT:

			SS_CheckTimeout(sessionId);

			break;

		case SS_SECTOR_MOVE:
		{
			int64_t playerId;
			ushort sectorX;
			ushort sectorY;

			*packet >> playerId >> sectorX >> sectorY;


			if (sectorX < 0 || sectorX >= 50 || sectorY < 0 || sectorY >= 50)
				__debugbreak();



			SS_ReqSectorMove(sessionId, playerId, sectorX, sectorY, player);
		}
		break;

		case SS_CHAT:
		{
			int64_t playerId;
			Chat chat;
			std::vector<unsigned short> sectorX;
			std::vector<unsigned short> sectorY;

			*packet >> playerId >> chat >> sectorX >> sectorY;

			SS_ReqChat(sessionId, playerId, chat, sectorX, sectorY);
		}
		break;

		default:

			__debugbreak();

		}

		Player* player1;

		auto it = tls_SessionIdToPlayer_.find(sessionId);

		if (it != tls_SessionIdToPlayer_.end())
		{
			player1 = it->second;
			player1->lastRecvTime_ = GetTickCount64();
		}


		/// TODO: 헤더 길이가 잘못 들어오는 상황을 어떻게 대처할지?
		if (packet != nullptr)
		{
			if (packet->IsError())
			{
				__debugbreak();
			}
		}

		if (packet != nullptr)
			packet->Free();

		messagePool_.Free(message);
		//delete message;
	}
}


void ChatServer::CreatePlayer(uint64_t sessionId)
{
	mapIdLock_.lock();

	/// mapId 추가
	sessionIdToMapId_[sessionId] = INITIAL_MAP_ID;

	mapIdLock_.unlock();

	Message* message = messagePool_.Alloc();
	//Message* message = new Message();

	message->Initialize(sessionId, nullptr, SS_CREATE_PLAYER);

	messageQueue_[INITIAL_MAP_ID].Enqueue(message);
}

void ChatServer::RemovePlayer(uint64_t sessionId)
{
	mapIdLock_.lock();

	auto it = sessionIdToMapId_.find(sessionId);

	if (it == sessionIdToMapId_.end())
	{
		mapIdLock_.unlock();

		return;
	}

	int mapId = it->second;

	/// mapId 먼저 확인 후 삭제
	sessionIdToMapId_.erase(sessionId);

	mapIdLock_.unlock();


	Message* message = messagePool_.Alloc();
	//Message* message = new Message();

	message->Initialize(sessionId, nullptr, SS_REMOVE_PLAYER);

	messageQueue_[mapId].Enqueue(message);
}

void ChatServer::CheckTimeout()
{
	for (int i = 0; i < MAP_COUNT; i++)
	{
		Message* message = messagePool_.Alloc();
		//Message* message = new Message();

		message->Initialize(0, nullptr, SS_CHECK_TIMEOUT);

		messageQueue_[i].Enqueue(message);
	}
}


void ChatServer::SS_CreatePlayer(uint64_t sessionId)
{
	Player* player = playerPool_.Alloc();
	//Player* player = new Player();

	player->Initialize(sessionId);

	tls_SessionIdToPlayer_.insert({ player->sessionId_ , player });
}

void ChatServer::SS_RemovePlayer(uint64_t sessionId)
{
	auto it = tls_SessionIdToPlayer_.find(sessionId);

	if (it == tls_SessionIdToPlayer_.end())
	{
		return;
	}

	Player* player = it->second;

	tls_SessionIdToPlayer_.erase(sessionId);

	if (player->isLogin_)
	{
		playerCount_--;

		tls_PlayerMap_.erase({ player->playerId_ });

		if (player->isInitSector_)
		{
			int mapX = SectorToMapX(player->sectorX_);
			int mapY = SectorToMapY(player->sectorY_);

			worldMap_[mapY][mapX].remove(player);
		}
	}

	// TODO: 다른 플레이어들에게 퇴장 알림

	playerPool_.Free(player);
	//delete player;
}

void ChatServer::SS_CheckTimeout(uint64_t sessionId)
{
	// 타임아웃 플레이어 세션 종료
	for (auto& pm : tls_PlayerMap_)
	{
		Player* p = pm.second;

		uint64_t timeDiff = GetTickCount64() - p->lastRecvTime_;

		if (timeDiff > TIMEOUT_MS)
		{
			Disconnect(p->sessionId_);
		}
	}
}
