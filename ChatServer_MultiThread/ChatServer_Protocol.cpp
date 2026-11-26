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

		uint64_t sessionId = message->sessionId_;
		Packet* packet = message->packet_;
		ushort protocol = message->protocol_;
		bool success;

		procMessageCount_++;

		switch (protocol)
		{
		case CS_NETWORK_PACKET:
		{
			messageCount_CS_NETWORK_PACKET_++;
			success = PacketProc(sessionId, packet);

			break;
		}

		case IN_CREATE_PLAYER:

			messageCount_SS_CreatePlayer_++;
			success = SS_CreatePlayer(sessionId);
			break;

		case IN_REMOVE_PLAYER:

			messageCount_SS_RemovePlayer_++;
			success = SS_RemovePlayer(sessionId);
			break;

		case SS_SECTOR_MOVE:

			success = SS_ReqSectorMove(sessionId, packet);
			break;

		case SS_CHAT:

			success = SS_ReqChat(sessionId, packet);
			break;

		default:

			__debugbreak();
		}

		if (!success)
		{
			Disconnect(sessionId);
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
	message->Initialize(sessionId, nullptr, IN_CREATE_PLAYER);

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
	message->Initialize(sessionId, nullptr, IN_REMOVE_PLAYER);

	messageQueue_[player->mapId_].Enqueue(message);
}

bool ChatServer_Multi::SS_CreatePlayer(uint64_t sessionId)
{
	Player* player = playerPool_.Alloc();

	player->Initialize(sessionId);

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		sessionIdToPlayer_.insert({ player->sessionId_ , player });
	}

	return true;
}

bool ChatServer_Multi::SS_RemovePlayer(uint64_t sessionId)
{
	Player* player;

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		auto it = sessionIdToPlayer_.find(sessionId);

		if (it == sessionIdToPlayer_.end())
			return false;

		player = it->second;

		sessionIdToPlayer_.erase(sessionId);
	}

	{
		std::lock_guard<std::mutex> lock(playerMapLock_);

		playerMap_.erase({ player->playerId_ });
	}

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
	return true;
}

void ChatServer_Multi::CreatePkt_SS_ReqSectorMove(Message* message, uint64_t sessionId, int64_t playerId, unsigned short sectorX, unsigned short sectorY)
{
	Packet* packet = packetPool_.Alloc();
	packet->Initialize();

	Pkt_SectorMove* buffer = reinterpret_cast<Pkt_SectorMove*>(packet->GetBodyPtr());

	buffer->playerId_ = playerId;
	buffer->sectorX_ = sectorX;
	buffer->sectorY_ = sectorY;

	message->Initialize(sessionId, packet, SS_SECTOR_MOVE);
}

void ChatServer_Multi::CreatePkt_SS_ReqChat(Message* message, uint64_t sessionId, int64_t playerId,
	Email email, Nickname nickname, Chat& chat, unsigned short sectorX, unsigned short sectorY)
{
	Packet* packet = packetPool_.Alloc();
	packet->Initialize();

	Pkt_Chat* buffer = reinterpret_cast<Pkt_Chat*>(packet->GetBodyPtr());

	buffer->playerId_ = playerId;
	buffer->email_ = email;
	buffer->nickname_ = nickname;
	// chat 배열 전체 복사
	buffer->chat_ = chat;
	buffer->sectorX_ = sectorX;
	buffer->sectorY_ = sectorY;

	message->Initialize(sessionId, packet, SS_CHAT);
}

bool ChatServer_Multi::SS_ReqSectorMove(uint64_t sessionId, Packet* packet)
{
	Pkt_SectorMove* pkt = reinterpret_cast<Pkt_SectorMove*>(packet->GetBodyPtr());

	int64_t playerId = pkt->playerId_;
	ushort sectorX = pkt->sectorX_;
	ushort sectorY = pkt->sectorY_;
	Player* player;

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		auto it = sessionIdToPlayer_.find(sessionId);

		if (it == sessionIdToPlayer_.end())
		{
			return false;
		}

		player = it->second;

		player->mapId_ = tls_MapId_;
	}

	//playerMap_[sessionId] = player;

	if (sectorX < 0 || sectorX >= 50 || sectorY < 0 || sectorY >= 50)
		__debugbreak();

	player->sectorX_ = sectorX;
	player->sectorY_ = sectorY;

	worldMap_[SectorToMapY(player->sectorY_)][SectorToMapX(player->sectorX_)].push_back(player);

	g_RpcProxy.SC_ResSectorMove(sessionId, player->playerId_, player->sectorX_, player->sectorY_);
	return true;
}

bool ChatServer_Multi::SS_ReqChat(uint64_t sessionId, Packet* packet)
{
	Pkt_Chat* pkt = reinterpret_cast<Pkt_Chat*>(packet->GetBodyPtr());

	int64_t playerId = pkt->playerId_;
	std::vector<uint64_t> sessionIds;

	for (auto& target : worldMap_[SectorToMapY(pkt->sectorY_)][SectorToMapX(pkt->sectorX_)])
	{
		if (target->isLogin_)
		{
			//g_RpcProxy.ResMessage(target->sessionId_, player->playerId_, player->email_, player->nickname_, chat);
			sessionIds.push_back(target->sessionId_);
		}
	}

	/// packet 재활용
	g_RpcProxy.SC_ResChat(sessionIds, pkt->playerId_, pkt->email_, pkt->nickname_, pkt->chat_);
	return true;
}
