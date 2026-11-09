#include "ChatServer.h"
#include "RpcModule.h"
#include "RPC/RpcProxy.h"
#include "Utils/Logger.h"


bool ChatServer::CS_ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey)
{
	auto it = tls_SessionIdToPlayer_.find(sessionId);

	if (it == tls_SessionIdToPlayer_.end())
	{
		return false;
	}

	Player* player = it->second;

	// 이미 로그인 완료 시 실패 패킷만 보냄
	if (player->isLogin_)
	{
		g_RpcProxy.SC_ResLogin(sessionId, ELOGIN::FAIL, playerId);

		return true;
	}


	auto it2 = tls_PlayerMap_.find(playerId);

	// 중복 접속 시 기존 접속 종료
	if (it2 != tls_PlayerMap_.end())
	{
		Player* alreadyLoginedPlayer = tls_PlayerMap_[playerId];

		alreadyLoginedPlayer->isLogin_ = false;
		playerCount_--;

		// 반드시 여기서 삭제
		tls_PlayerMap_.erase({ alreadyLoginedPlayer->playerId_ });


		if (alreadyLoginedPlayer->isInitSector_)
		{
			int mapX = SectorToMapX(alreadyLoginedPlayer->sectorX_);
			int mapY = SectorToMapY(alreadyLoginedPlayer->sectorY_);

			worldMap_[mapY][mapX].remove(alreadyLoginedPlayer);
		}

		++dupLoginCountTotal_;

		// Disconnect 이후론 로직 없어야함
		Disconnect(alreadyLoginedPlayer->sessionId_);
	}

	// TODO: 세션 키 인증

	player->Initialize(playerId, loginId, nickname);
	tls_PlayerMap_.insert({ playerId, player });


	playerCount_++;

	g_RpcProxy.SC_ResLogin(sessionId, ELOGIN::SUCCESS, playerId);

	return true;
}

bool ChatServer::CS_ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY)
{
	auto it = tls_SessionIdToPlayer_.find(sessionId);

	if (it == tls_SessionIdToPlayer_.end())
	{
		return false;
	}

	Player* player = it->second;

	if (!player->isLogin_)
		return false;

	if (playerId != player->playerId_)
	{
		//__debugbreak();
		//LOG(L"reqmessage playerid error");

		return false;
	}

	if (sectorX < 0 || sectorX >= SECTOR_WIDTH || sectorY < 0 || sectorY >= SECTOR_HEIGHT)
	{
		LOG(L"not valid sector");

		return false;
	}




	int mapId = SectorToMapId(sectorX, sectorY);


	if (player->isInitSector_)
	{
		int oldMapX = SectorToMapX(player->sectorX_);
		int oldMapY = SectorToMapY(player->sectorY_);


		worldMap_[oldMapY][oldMapX].remove(player);
	}
	else
	{
		player->isInitSector_ = true;
	}

	
	if (mapId == tls_MapId_)
	{
		player->sectorX_ = sectorX;
		player->sectorY_ =sectorY;

		worldMap_[SectorToMapY(player->sectorY_)][SectorToMapX(player->sectorX_)].push_back(player);

		g_RpcProxy.SC_ResSectorMove(sessionId, player->playerId_, player->sectorX_, player->sectorY_);

		return true;
	}
	else
	{
		//mapIdLock_.lock();

		//sessionIdToMapId_.erase(sessionId);

		//sessionIdToMapId_[sessionId] = mapId;

		//mapIdLock_.unlock();


		/// TODO: 수정
		tls_SessionIdToPlayer_.erase(sessionId);
		tls_PlayerMap_.erase(player->playerId_);


		Message* message = messagePool_.Alloc();
		//Message* message = new Message();

		Packet* packet = packetPool_.Alloc();
		packet->Initialize(sizeof(NET_HEADER));

		*packet << playerId << sectorX << sectorY;

		/// player 추가
		message->Initialize(sessionId, packet, SS_SECTOR_MOVE, player);

		messageQueue_[mapId].Enqueue(message);

		return true;
	}
}

bool ChatServer::CS_ReqChat(uint64_t sessionId, int64_t playerId, Chat& chat)
{
	auto it = tls_SessionIdToPlayer_.find(sessionId);

	if (it == tls_SessionIdToPlayer_.end())
	{
		return false;
	}

	Player* player = it->second;

	if (!player->isLogin_)
		return false;

	if (!player->isInitSector_)
	{
		LOG(L"not init sector");

		return false;
	}

	if (playerId != player->playerId_)
	{
		//__debugbreak();
		//LOG(L"reqmessage playerid error");

		return false;
	}

	std::vector<uint64_t> sessionIds;

	std::vector<unsigned short> sectorX[4];
	std::vector<unsigned short> sectorY[4];



	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			unsigned short sectorNX = player->sectorX_ + dx[i];
			unsigned short sectorNY = player->sectorY_ + dy[j];

			if (sectorNX < 0 || sectorNX >= SECTOR_WIDTH || sectorNY < 0 || sectorNY >= SECTOR_HEIGHT)
				continue;

			int mapId = SectorToMapId(sectorNX, sectorNY);

			if (mapId == tls_MapId_)
			{
				for (auto& target : worldMap_[SectorToMapY(sectorNY)][SectorToMapX(sectorNX)])
				{
					//g_RpcProxy.ResMessage(target->sessionId_, player->playerId_, player->loginId_, player->nickname_, chat);
					if (target->isLogin_)
					{
						sessionIds.push_back(target->sessionId_);
					}
				}
			}
			else
			{
				sectorX[mapId].push_back(sectorNX);
				sectorY[mapId].push_back(sectorNY);
			}
		}
	}

	/// packet 재활용
	g_RpcProxy.SC_ResChat(sessionIds, player->playerId_, player->loginId_, player->nickname_, chat);


	for (int i = 0; i < 4; i++)
	{
		//if (i == tls_MapId_)
		//	continue;

		if (sectorX[i].size() == 0)
			continue;

		Message* message = messagePool_.Alloc();
		//Message* message = new Message();

		Packet* packet = packetPool_.Alloc();
		packet->Initialize(sizeof(NET_HEADER));

		*packet << playerId << chat << sectorX[i] << sectorY[i];

		message->Initialize(sessionId, packet, SS_CHAT);

		messageQueue_[i].Enqueue(message);
	}

	return true;
}

bool ChatServer::CS_ReqHeartbeat(uint64_t sessionId)
{
	auto it = tls_SessionIdToPlayer_.find(sessionId);

	if (it == tls_SessionIdToPlayer_.end())
	{
		return false;
	}

	Player* player = it->second;

	if (!player->isLogin_)
		return false;

	return true;
}
