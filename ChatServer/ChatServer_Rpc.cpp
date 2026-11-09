#include "ChatServer.h"
#include "RpcModule.h"
#include "RPC/RpcProxy.h"
#include "Utils/Logger.h"


bool ChatServer::CS_ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey)
{
	auto it = sessionIdToPlayer_.find(sessionId);

	if (it == sessionIdToPlayer_.end())
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


	auto it2 = playerMap_.find(playerId);

	// 중복 접속 시 기존 접속 종료
	if (it2 != playerMap_.end())
	{
		Player* alreadyLoginedPlayer = playerMap_[playerId];

		alreadyLoginedPlayer->isLogin_ = false;
		playerCount_--;

		// 반드시 여기서 삭제
		playerMap_.erase({ alreadyLoginedPlayer->playerId_ });

		if (alreadyLoginedPlayer->isInitSector_)
		{
			sector_[alreadyLoginedPlayer->sectorY_][alreadyLoginedPlayer->sectorX_].remove(alreadyLoginedPlayer);
		}

		++dupLoginCountTotal_;

		// Disconnect 이후론 로직 없어야함
		Disconnect(alreadyLoginedPlayer->sessionId_);
	}

	// TODO: 세션 키 인증

	player->Initialize(playerId, loginId, nickname);
	playerMap_.insert({ playerId, player });

	playerCount_++;

	g_RpcProxy.SC_ResLogin(sessionId, ELOGIN::SUCCESS, playerId);

	return true;
}

bool ChatServer::CS_ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY)
{
	auto it = sessionIdToPlayer_.find(sessionId);

	if (it == sessionIdToPlayer_.end())
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

	//unsigned int now = GetTickCount();

	//// 1초 내에 다시 채팅 보냈을시
	//if (now - player->lastChatTime_ < 1000)
	//{
	//	player->curChatCount_++;
	//}

	//// 채팅 도배 시 확인시점으로부터 5초간 금지
	//if (now - player->lastChatTime_ >= 5000)
	//{
	//	player->curChatCount_ = 0;
	//}

	//// 채팅 도배 금지
	//if (player->curChatCount_ > 5)
	//{
	//	std::string m = u8"잠시 후 다시 채팅해주세요.";
	//	g_RpcProxy.ResChat(sessionId, player->playerId_, m);

	//	return true;
	//}

	//player->lastChatTime_ = now;

	std::vector<uint64_t> sessionIds;

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
				//g_RpcProxy.ResMessage(target->sessionId_, player->playerId_, player->loginId_, player->nickname_, chat);
				if (target->isLogin_)
				{
					sessionIds.push_back(target->sessionId_);
				}
			}
		}
	}

	/// packet 재활용
	g_RpcProxy.SC_ResChat(sessionIds, player->playerId_, player->loginId_, player->nickname_, chat);

	return true;
}

bool ChatServer::CS_ReqHeartbeat(uint64_t sessionId)
{
	auto it = sessionIdToPlayer_.find(sessionId);

	if (it == sessionIdToPlayer_.end())
	{
		return false;
	}

	Player* player = it->second;

	if (!player->isLogin_)
		return false;

	return true;
}
