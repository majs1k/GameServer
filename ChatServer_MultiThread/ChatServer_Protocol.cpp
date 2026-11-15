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



			SS_ReqSectorMove(sessionId, playerId, sectorX, sectorY);
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

		/// 수정 필요
		auto it = sessionIdToPlayer_.find(sessionId);

		if (it != sessionIdToPlayer_.end())
		{
			player1 = it->second;
			player1->lastRecvTime_ = GetTickCount64();
		}

		if (packet != nullptr)
			packet->Free();

		messagePool_.Free(message);
		//delete message;
	}
}


void ChatServer::CreatePlayer(uint64_t sessionId)
{
	Message* message = messagePool_.Alloc();
	//Message* message = new Message();

	message->Initialize(sessionId, nullptr, SS_CREATE_PLAYER);

	messageQueue_[INITIAL_MAP_ID].Enqueue(message);
}

void ChatServer::RemovePlayer(uint64_t sessionId)
{
	Player* player;

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		auto it = sessionIdToPlayer_.find(sessionId);

		if (it == sessionIdToPlayer_.end())
		{
			return;
		}

		player = it->second;
	}

	Message* message = messagePool_.Alloc();
	//Message* message = new Message();

	message->Initialize(sessionId, nullptr, SS_REMOVE_PLAYER);

	messageQueue_[player->mapId_].Enqueue(message);
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

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		sessionIdToPlayer_.insert({ player->sessionId_ , player });
	}
}

void ChatServer::SS_RemovePlayer(uint64_t sessionId)
{
	Player* player;

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		auto it = sessionIdToPlayer_.find(sessionId);

		if (it == sessionIdToPlayer_.end())
		{
			return;
		}

		player = it->second;

		sessionIdToPlayer_.erase(sessionId);
	}

	tls_PlayerMap_.erase({ player->playerId_ });

	if (player->isLogin_)
	{
		playerCount_--;

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
