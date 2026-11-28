#include "ChatServer.h"
#include "Utils/Logger.h"


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

bool ChatServer::IN_CreatePlayer(uint64_t sessionId)
{
	Player* player = playerPool_.Alloc();
	player->Initialize(sessionId);

	sessionIdToPlayer_.insert({ player->sessionId_ , player });
	return true;
}

bool ChatServer::IN_RemovePlayer(uint64_t sessionId)
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
