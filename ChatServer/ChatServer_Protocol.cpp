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
		bool isSuccess;

		procMessageCount_++;

		switch (protocol)
		{
		case CS_NETWORK_PACKET:
		{
			messageCount_CS_NETWORK_PACKET_++;
			isSuccess = PacketProc(sessionId, packet);
			break;
		}

		case SS_CREATE_PLAYER:

			messageCount_SS_CreatePlayer_++;
			isSuccess = SS_CreatePlayer(sessionId);
			break;

		case SS_REMOVE_PLAYER:

			messageCount_SS_RemovePlayer_++;
			isSuccess = SS_RemovePlayer(sessionId);
			break;

		case SS_CHECK_TIMEOUT:

			isSuccess = SS_CheckTimeout();
			break;

		default:

			__debugbreak();
		}

		if (!isSuccess)
		{
			Disconnect(sessionId);
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


void ChatServer::CreatePlayer(uint64_t sessionId)
{
	Message* message = messagePool_.Alloc();
	message->Initialize(sessionId, nullptr, SS_CREATE_PLAYER);

	messageQueue_.Enqueue(message);
}

void ChatServer::RemovePlayer(uint64_t sessionId)
{
	Message* message = messagePool_.Alloc();
	message->Initialize(sessionId, nullptr, SS_REMOVE_PLAYER);

	messageQueue_.Enqueue(message);
}

void ChatServer::CheckTimeout()
{
	Message* message = messagePool_.Alloc();
	message->Initialize(0, nullptr, SS_CHECK_TIMEOUT);

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

bool ChatServer::SS_CheckTimeout()
{
	for (auto& pm : playerMap_)
	{
		Player* p = pm.second;
		uint64_t timeDiff = GetTickCount64() - p->lastRecvTime_;

		if (timeDiff > TIMEOUT_MS)
		{
			timeoutCountTotal_++;
			Disconnect(p->sessionId_);
		}
	}

	return true;
}
