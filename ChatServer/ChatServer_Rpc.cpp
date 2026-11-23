#include "ChatServer.h"
#include "RpcModule.h"
#include "RPC/RpcProxy.h"
#include "Utils/Logger.h"
#include "Utils/Profiler.h"


bool ChatServer::CS_ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey)
{
	auto it = sessionIdToPlayer_.find(sessionId);

	if (it == sessionIdToPlayer_.end())
		return false;

	Player* player = it->second;

	// 이미 로그인 완료 시 실패 패킷만 보냄
	if (player->isLogin_)
	{
		g_RpcProxy.SC_ResLogin(sessionId, ELOGIN::FAIL, playerId);

		return true;
	}

	auto it2 = playerMap_.find(playerId);

	// 중복 접속 시 기존 접속만 종료
	// 어택 x 더미를 로컬에서 같이 테스트시 발생 불가 (RST 유실 발생 안함)
	if (it2 != playerMap_.end())
	{
		Player* alreadyLoginedPlayer = it2->second;

		/// 더미가 기존 로그인만 끊는 구조라 반드시 여기서 삭제
		playerMap_.erase({ alreadyLoginedPlayer->playerId_ });

		if (alreadyLoginedPlayer->isInitSector_)
		{
			sector_[alreadyLoginedPlayer->sectorY_][alreadyLoginedPlayer->sectorX_].remove(alreadyLoginedPlayer);
		}

		dupLoginCountTotal_++;
		Disconnect(alreadyLoginedPlayer->sessionId_);
		LOG("CHATSERVER", ELOG::SYSTEM_LEVEL, "duplicate login / id=%d", playerId);
	}

	// + 세션 키 인증

	player->Initialize(playerId, loginId, nickname);
	playerMap_.insert({ playerId, player });
	// 로그인 성공 시 플레이어로 카운트
	playerCount_++;

	g_RpcProxy.SC_ResLogin(sessionId, ELOGIN::SUCCESS, playerId);
	return true;
}

bool ChatServer::CS_ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY)
{
	auto it = sessionIdToPlayer_.find(sessionId);

	if (it == sessionIdToPlayer_.end())
		return false;

	Player* player = it->second;

	if (!player->isLogin_)
		return false;

	if (playerId != player->playerId_)
	{
		//__debugbreak();
		//LOG("CHATSERVER", ELOG::ERROR_LEVEL, "req_sectormove id error / id: %d", playerId);
		return false;
	}

	if (sectorX < 0 || sectorX >= SECTOR_WIDTH || sectorY < 0 || sectorY >= SECTOR_HEIGHT)
	{
		//LOG("CHATSERVER", ELOG::ERROR_LEVEL, "not valid sector / id=%d / sectorX=%d / secotrY=%d", player->playerId_, sectorX, sectorY);
		return false;
	}

	if (!player->isInitSector_)
	{
		player->isInitSector_ = true;
	}
	else
	{
		// 만약 1칸씩만 이동이 가능하다면 추가
		//if (abs(sectorX - player->sectorX_) > 1 || abs(sectorY - player->sectorY_) > 1)
		//{
		//	return false;
		//}

		sector_[player->sectorY_][player->sectorX_].remove(player);
	}


	player->sectorX_ = sectorX;
	player->sectorY_ = sectorY;

	sector_[player->sectorY_][player->sectorX_].push_back(player);

	g_RpcProxy.SC_ResSectorMove(sessionId, player->playerId_, player->sectorX_, player->sectorY_);
	return true;
}

bool ChatServer::CS_ReqChat(uint64_t sessionId, int64_t playerId, Chat& chat)
{
	auto it = sessionIdToPlayer_.find(sessionId);

	if (it == sessionIdToPlayer_.end())
		return false;

	Player* player = it->second;

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
	sessionIds.reserve(1000);

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			unsigned short sectorNX = player->sectorX_ + dx[i];
			unsigned short sectorNY = player->sectorY_ + dy[j];

			if (sectorNX < 0 || sectorNX >= SECTOR_WIDTH || sectorNY < 0 || sectorNY >= SECTOR_HEIGHT)
				continue;

			for (auto& target : sector_[sectorNY][sectorNX])
			{
				if (target->isLogin_)
				{
					//g_RpcProxy.ResMessage(target->sessionId_, player->playerId_, player->loginId_, player->nickname_, chat);
					sessionIds.push_back(target->sessionId_);
				}
			}
		}
	}

	/// packet 재활용
	g_RpcProxy.SC_ResChat(sessionIds, player->playerId_, player->loginId_, player->nickname_, chat);

	/// 디버깅
	//if (chat.chat_[0] == L'=')
	//{
	//	std::lock_guard<std::mutex> lock(debuggingLock_);
	//	debuggingMap_[sessionId] = 1;
	//}

	return true;
}

bool ChatServer::CS_ReqHeartbeat(uint64_t sessionId)
{
	auto it = sessionIdToPlayer_.find(sessionId);

	if (it == sessionIdToPlayer_.end())
		return false;

	Player* player = it->second;

	if (!player->isLogin_)
		return false;

	return true;
}
