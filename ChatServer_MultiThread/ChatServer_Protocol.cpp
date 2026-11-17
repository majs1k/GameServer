#include "ChatServer_Multi.h"
#include "RpcModule.h"


void ChatServer_Multi::ProcessMessageQueue()
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
			messageCount_CS_NETWORK_PACKET_++;

			if (!PacketProc(sessionId, packet))
			{
				Disconnect(sessionId);
			}

			break;
		}

		case SS_CREATE_PLAYER:

			messageCount_SS_CreatePlayer_++;
			SS_CreatePlayer(sessionId);
			break;

		case SS_REMOVE_PLAYER:

			messageCount_SS_RemovePlayer_++;
			SS_RemovePlayer(sessionId);
			break;

		case SS_CHECK_TIMEOUT:

			SS_CheckTimeout();
			break;

		case SS_SECTOR_MOVE:

			SS_ReqSectorMove(sessionId, packet);
			break;

		case SS_CHAT:

			SS_ReqChat(sessionId, packet);
			break;

		default:

			__debugbreak();
		}

		auto it = sessionIdToPlayer_.find(sessionId);

		if (it != sessionIdToPlayer_.end())
		{
			Player* player = it->second;
			player->lastRecvTime_ = GetTickCount64();
		}

		if (packet != nullptr)
		{
			packet->Free();
		}

		messagePool_.Free(message);
	}
}


void ChatServer_Multi::CreatePlayer(uint64_t sessionId)
{
	Message* message = messagePool_.Alloc();
	message->Initialize(sessionId, nullptr, SS_CREATE_PLAYER);

	messageQueue_[INITIAL_MAP_ID].Enqueue(message);
}

void ChatServer_Multi::RemovePlayer(uint64_t sessionId)
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
	message->Initialize(sessionId, nullptr, SS_REMOVE_PLAYER);

	messageQueue_[player->mapId_].Enqueue(message);
}

void ChatServer_Multi::CheckTimeout()
{
	for (int i = 0; i < MAP_COUNT; i++)
	{
		Message* message = messagePool_.Alloc();
		message->Initialize(0, nullptr, SS_CHECK_TIMEOUT);

		messageQueue_[i].Enqueue(message);
	}
}


void ChatServer_Multi::SS_CreatePlayer(uint64_t sessionId)
{
	Player* player = playerPool_.Alloc();

	player->Initialize(sessionId);

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		sessionIdToPlayer_.insert({ player->sessionId_ , player });
	}
}

void ChatServer_Multi::SS_RemovePlayer(uint64_t sessionId)
{
	Player* player;

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		auto it = sessionIdToPlayer_.find(sessionId);

		if (it == sessionIdToPlayer_.end())
			return;

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

	// + 다른 플레이어들에게 퇴장 알림

	playerPool_.Free(player);
}

void ChatServer_Multi::SS_CheckTimeout()
{
	for (auto& pm : tls_PlayerMap_)
	{
		Player* p = pm.second;
		uint64_t timeDiff = GetTickCount64() - p->lastRecvTime_;

		if (timeDiff > TIMEOUT_MS)
		{
			timeoutCountTotal_++;
			Disconnect(p->sessionId_);
		}
	}
}

void ChatServer_Multi::CreatePkt_SS_ReqSectorMove(Message* message, uint64_t sessionId, int64_t playerId, unsigned short sectorX, unsigned short sectorY)
{
	Packet* packet = packetPool_.Alloc();
	packet->Initialize();

	Pkt_SectorMove* buffer = reinterpret_cast<Pkt_SectorMove*>(packet->GetBufferPtr());

	buffer->playerId_ = playerId;
	buffer->sectorX_ = sectorX;
	buffer->sectorY_ = sectorY;

	message->Initialize(sessionId, packet, SS_SECTOR_MOVE);
}

void ChatServer_Multi::CreatePkt_SS_ReqChat(Message* message, uint64_t sessionId, int64_t playerId, LoginId loginId, Nickname nickname, Chat& chat, unsigned short sectorX, unsigned short sectorY)
{
	Packet* packet = packetPool_.Alloc();
	packet->Initialize();

	Pkt_Chat* buffer = reinterpret_cast<Pkt_Chat*>(packet->GetBufferPtr());

	buffer->playerId_ = playerId;
	buffer->loginId_ = loginId;
	buffer->nickname_ = nickname;
	buffer->chat_ = chat;
	buffer->sectorX_ = sectorX;
	buffer->sectorY_ = sectorY;

	message->Initialize(sessionId, packet, SS_CHAT);
}

void ChatServer_Multi::SS_ReqSectorMove(uint64_t sessionId, Packet* packet)
{
	Pkt_SectorMove* pkt = reinterpret_cast<Pkt_SectorMove*>(packet->GetBufferPtr());

	int64_t playerId = pkt->playerId_;
	ushort sectorX = pkt->sectorX_;
	ushort sectorY = pkt->sectorY_;
	Player* player;

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		auto it = sessionIdToPlayer_.find(sessionId);

		if (it == sessionIdToPlayer_.end())
		{
			return;
		}

		player = it->second;

		player->mapId_ = tls_MapId_;
	}

	tls_PlayerMap_[sessionId] = player;

	if (sectorX < 0 || sectorX >= 50 || sectorY < 0 || sectorY >= 50)
		__debugbreak();

	player->sectorX_ = sectorX;
	player->sectorY_ = sectorY;

	worldMap_[SectorToMapY(player->sectorY_)][SectorToMapX(player->sectorX_)].push_back(player);

	g_RpcProxy.SC_ResSectorMove(sessionId, player->playerId_, player->sectorX_, player->sectorY_);
}

void ChatServer_Multi::SS_ReqChat(uint64_t sessionId, Packet* packet)
{
	Pkt_Chat* pkt = reinterpret_cast<Pkt_Chat*>(packet->GetBufferPtr());

	int64_t playerId = pkt->playerId_;
	std::vector<uint64_t> sessionIds;

	for (auto& target : worldMap_[SectorToMapY(pkt->sectorY_)][SectorToMapX(pkt->sectorX_)])
	{
		if (target->isLogin_)
		{
			//g_RpcProxy.ResMessage(target->sessionId_, player->playerId_, player->loginId_, player->nickname_, chat);
			sessionIds.push_back(target->sessionId_);
		}
	}

	/// packet 재활용
	g_RpcProxy.SC_ResChat(sessionIds, pkt->playerId_, pkt->loginId_, pkt->nickname_, pkt->chat_);
}
