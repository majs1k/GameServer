#include "ChatServer.h"
#include "Utils/Logger.h"

void ChatServer::ProcessMessageQueue()
{
	while (1)
	{
		Message* message;
		bool ret = messageQueue_.Dequeue(message);

		if (ret == false)
			break;

		uint64_t sessionId = message->sessionId_;
		Packet* packet = message->packet_;
		ushort protocol = message->protocol_;
		bool success;

		procMessageCount_++;

		switch (protocol)
		{
		case CS_NETWORK_PACKET_CHAT_SERVER:
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


void ChatServer::CreatePlayer(uint64_t sessionId)
{
	Message* message = messagePool_.Alloc();
	message->Initialize(sessionId, nullptr, IN_CREATE_PLAYER);

	messageQueue_.Enqueue(message);
}

void ChatServer::RemovePlayer(uint64_t sessionId)
{
	Message* message = messagePool_.Alloc();
	message->Initialize(sessionId, nullptr, IN_REMOVE_PLAYER);

	messageQueue_.Enqueue(message);
}

bool ChatServer::SS_CreatePlayer(uint64_t sessionId)
{
	Player* player = playerPool_.Alloc();
	player->Initialize(sessionId);

	sessionIdToPlayer_.insert({ player->sessionId_ , player });
	return true;
}

bool ChatServer::SS_RemovePlayer(uint64_t sessionId)
{
	auto it = sessionIdToPlayer_.find(sessionId);

	if (it == sessionIdToPlayer_.end())
		return false;

	Player* player = it->second;

	sessionIdToPlayer_.erase(sessionId);

	if (player->isLogin_)
	{
		playerCount_--;

		playerMap_.erase({ player->playerId_ });

		if (player->isInitSector_)
		{
			sector_[player->sectorY_][player->sectorX_].remove(player);
		}
	}

	// + 다른 플레이어들에게 퇴장 알림

	playerPool_.Free(player);
	return true;
}
