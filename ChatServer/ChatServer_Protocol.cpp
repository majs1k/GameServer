#include "ChatServer.h"


void ChatServer::ProcessMessageQueue()
{
	while (1)
	{
		Message* message;
		bool ret = messageQueue_.Dequeue(message);

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


		default:

			__debugbreak();

		}

		auto it = sessionIdToPlayer_.find(sessionId);

		if (it != sessionIdToPlayer_.end())
		{
			Player* player = it->second;
			player->lastRecvTime_ = GetTickCount64();
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
	Message* message = messagePool_.Alloc();
	//Message* message = new Message();

	message->Initialize(sessionId, nullptr, SS_CREATE_PLAYER);

	messageQueue_.Enqueue(message);
}

void ChatServer::RemovePlayer(uint64_t sessionId)
{
	Message* message = messagePool_.Alloc();
	//Message* message = new Message();

	message->Initialize(sessionId, nullptr, SS_REMOVE_PLAYER);

	messageQueue_.Enqueue(message);
}

void ChatServer::CheckTimeout()
{
	Message* message = messagePool_.Alloc();
	//Message* message = new Message();

	message->Initialize(0, nullptr, SS_CHECK_TIMEOUT);

	messageQueue_.Enqueue(message);
}


void ChatServer::SS_CreatePlayer(uint64_t sessionId)
{
	Player* player = playerPool_.Alloc();
	//Player* player = new Player();

	player->Initialize(sessionId);

	sessionIdToPlayer_.insert({ player->sessionId_ , player });
}

void ChatServer::SS_RemovePlayer(uint64_t sessionId)
{
	auto it = sessionIdToPlayer_.find(sessionId);

	if (it == sessionIdToPlayer_.end())
	{
		return;
	}

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

	// TODO: 다른 플레이어들에게 퇴장 알림

	playerPool_.Free(player);
	//delete player;
}

void ChatServer::SS_CheckTimeout(uint64_t sessionId)
{
	// 타임아웃 플레이어 세션 종료
	for (auto& pm : playerMap_)
	{
		Player* p = pm.second;

		uint64_t timeDiff = GetTickCount64() - p->lastRecvTime_;

		if (timeDiff > TIMEOUT_MS)
		{
			Disconnect(p->sessionId_);
		}
	}
}
