#include "ChatServer.h"
#include "RpcModule.h"
#include "RPC/RpcProxy.h"
#include "Utils/Logger.h"


bool ChatServer::CS_ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey)
{
	Player* player = sessionIdToPlayer_[sessionId];

	// 이미 로그인 완료 시 실패 패킷만 보냄
	if (player->isLogin_)
	{
		g_RpcProxy.SC_ResLogin(sessionId, ELOGIN::FAIL, playerId);

		return true;
	}

	// 중복 접속 시 기존 접속 종료
	if (playerMap_.count(playerId))
	{
		Player* alreadyLoginedPlayer = playerMap_[playerId];

		alreadyLoginedPlayer->isLogin_ = false;
		playerCount_--;

		// 반드시 여기서 삭제
		playerMap_.erase({ alreadyLoginedPlayer->playerId_ });

		// Disconnect 이후론 로직 없어야함
		Disconnect(alreadyLoginedPlayer->sessionId_);
	}

	player->lastRecvTime_ = GetTickCount();

	// TODO: 세션 키 인증

	auto iter = sessionIdToPlayer_.find(sessionId);
	if (iter == sessionIdToPlayer_.end())
	{
		__debugbreak();
	}

	player->Initialize(sessionId, playerId, loginId, nickname);
	playerMap_.insert({ playerId, player });

	playerCount_++;

	g_RpcProxy.SC_ResLogin(sessionId, ELOGIN::SUCCESS, playerId);

	return true;
}

bool ChatServer::CS_ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY)
{
	Player* player = sessionIdToPlayer_[sessionId];

	if (!player->isLogin_)
		return false;

	player->lastRecvTime_ = GetTickCount();

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

		/// 디버깅용
		if (player == nullptr)
			__debugbreak();

		sector_[player->sectorY_][player->sectorX_].remove(player);
	}


	player->sectorX_ = sectorX;
	player->sectorY_ = sectorY;

	/// 디버깅용
	if (player == nullptr)
		__debugbreak();

	sector_[player->sectorY_][player->sectorX_].push_back(player);

	g_RpcProxy.SC_ResSectorMove(sessionId, player->playerId_, player->sectorX_, player->sectorY_);
}

bool ChatServer::CS_ReqMessage(uint64_t sessionId, int64_t playerId, Chat& chat)
{
	Player* player = sessionIdToPlayer_[sessionId];

	if (!player->isLogin_)
		return false;

	if (!player->isInitSector_)
	{
		LOG(L"not init sector");

		return false;
	}

	player->lastRecvTime_ = GetTickCount();


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
			int sectorNX = player->sectorX_ + dx[i];
			int sectorNY = player->sectorY_ + dy[j];

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
	g_RpcProxy.SC_ResMessage(sessionIds, player->playerId_, player->loginId_, player->nickname_, chat);

	return true;
}

bool ChatServer::CS_ReqHeartbeat(uint64_t sessionId)
{
	Player* player = sessionIdToPlayer_[sessionId];

	if (!player->isLogin_)
		return false;

	player->lastRecvTime_ = GetTickCount();

	return true;
}
