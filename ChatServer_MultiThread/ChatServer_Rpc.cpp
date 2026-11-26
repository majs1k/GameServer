#include "ChatServer_Multi.h"
#include "RpcModule.h"
#include "RPC/RpcProxy.h"
#include "Utils/Logger.h"


bool ChatServer_Multi::CS_ReqEnterChatServer(uint64_t sessionId, int64_t playerId, Email& email, Nickname& nickname, SessionKey& sessionKey)
{
	Player* player;

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		auto it = sessionIdToPlayer_.find(sessionId);

		if (it == sessionIdToPlayer_.end())
			return false;

		player = it->second;
	}

	// 이미 로그인 완료 시 실패 패킷만 보냄
	if (player->isLogin_)
	{
		g_RpcProxy.SC_ResEnterChatServer(sessionId, ELOGIN::FAIL, playerId);
		return true;
	}

	std::unordered_map<int64_t, Player*>::iterator it2;
	std::unordered_map<int64_t, Player*>::iterator it3;

	{
		std::lock_guard<std::mutex> lock(playerMapLock_);

		it2 = playerMap_.find(playerId);

		it3 = playerMap_.end();
	}


	// 중복 접속 시 기존 접속만 종료
	// 어택 x 더미를 로컬에서 같이 테스트시 발생 불가 (RST 유실 발생 안함)
	if (it2 != it3)
	{
		Player* alreadyLoginedPlayer = it2->second;

		{
			std::lock_guard<std::mutex> lock(playerMapLock_);

			// 반드시 여기서 삭제
			playerMap_.erase({ alreadyLoginedPlayer->playerId_ });
		}

		if (alreadyLoginedPlayer->isInitSector_)
		{
			int mapX = SectorToMapX(alreadyLoginedPlayer->sectorX_);
			int mapY = SectorToMapY(alreadyLoginedPlayer->sectorY_);

			worldMap_[mapY][mapX].remove(alreadyLoginedPlayer);
		}

		dupLoginCountTotal_++;
		Disconnect(alreadyLoginedPlayer->sessionId_);
	}

	// + 세션 키 인증

	player->Initialize(playerId, email, nickname);

	{
		std::lock_guard<std::mutex> lock(playerMapLock_);

		/// 초기 맵에서 생성
		playerMap_.insert({ playerId, player });
	}

	// 로그인 성공 시 플레이어로 카운트
	playerCount_++;

	g_RpcProxy.SC_ResEnterChatServer(sessionId, ELOGIN::SUCCESS, playerId);
	return true;
}

bool ChatServer_Multi::CS_ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY)
{
	Player* player;

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		auto it = sessionIdToPlayer_.find(sessionId);

		if (it == sessionIdToPlayer_.end())
			return false;

		player = it->second;
	}

	if (!player->isLogin_)
		return false;

	if (playerId != player->playerId_)
	{
		//__debugbreak();
		//LOG("CHATSERVER", ELOG::ERROR_LEVEL, "req_sectormove id error / id=%d", playerId);
		return false;
	}

	if (sectorX < 0 || sectorX >= SECTOR_WIDTH || sectorY < 0 || sectorY >= SECTOR_HEIGHT)
	{
		//LOG("CHATSERVER", ELOG::ERROR_LEVEL, "not valid sector / id=%d / sectorX=%d / secotrY=%d", player->playerId_, sectorX, sectorY);
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
		player->sectorY_ = sectorY;

		worldMap_[SectorToMapY(player->sectorY_)][SectorToMapX(player->sectorX_)].push_back(player);

		g_RpcProxy.SC_ResSectorMove(sessionId, player->playerId_, player->sectorX_, player->sectorY_);
		return true;
	}
	else
	{
		//playerMap_.erase(player->playerId_);

		Message* message = messagePool_.Alloc();
		CreatePkt_SS_ReqSectorMove(message, sessionId, playerId, sectorX, sectorY);

		messageQueue_[mapId].Enqueue(message);
		return true;
	}
}

bool ChatServer_Multi::CS_ReqChat(uint64_t sessionId, int64_t playerId, Chat& chat)
{
	Player* player;

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		auto it = sessionIdToPlayer_.find(sessionId);

		if (it == sessionIdToPlayer_.end())
			return false;

		player = it->second;
	}

	if (!player->isLogin_)
		return false;

	if (!player->isInitSector_)
	{
		//LOG("CHATSERVER", ELOG::ERROR_LEVEL, "not init sector / id=%d", player->playerId_);
		return false;
	}

	if (playerId != player->playerId_)
	{
		//__debugbreak();
		//LOG("CHATSERVER", ELOG::ERROR_LEVEL, "req_chat id error / id=%d", playerId);
		return false;
	}

	std::vector<uint64_t> sessionIds;

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
					if (target->isLogin_)
					{
						//g_RpcProxy.ResMessage(target->sessionId_, player->playerId_, player->email_, player->nickname_, chat);
						sessionIds.push_back(target->sessionId_);
					}
				}
			}
			else
			{
				/// 다른 맵(스레드)로 메세지 전달
				Message* message = messagePool_.Alloc();
				CreatePkt_SS_ReqChat(message, sessionId, player->playerId_, player->email_, player->nickname_, chat, sectorNX, sectorNY);

				messageQueue_[mapId].Enqueue(message);
			}
		}
	}

	/// packet 재활용
	g_RpcProxy.SC_ResChat(sessionIds, player->playerId_, player->email_, player->nickname_, chat);

	return true;
}

bool ChatServer_Multi::CS_ReqHeartbeat(uint64_t sessionId)
{
	Player* player;

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		auto it = sessionIdToPlayer_.find(sessionId);

		if (it == sessionIdToPlayer_.end())
			return false;

		player = it->second;
	}

	if (!player->isLogin_)
		return false;

	return true;
}
