#include "ChatServer.h"
#include "RpcModule.h"
#include "RPC/RpcProxy.h"
#include "Utils/TickController.h"


bool ChatServer::ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey)
{
	// 접속안된 플레이어 & 플레이어 중복 접속 차단
	if (sessionIdToPlayerId_.count(sessionId) == 0 || sessionIdToPlayerId_[sessionId] > 0)
	{
		g_RpcProxy.ResLogin(sessionId, ELOGIN::FAIL, playerId);

		return false;
	}

	// TODO: 세션 키 인증

	int64_t oldPlayerId = sessionIdToPlayerId_[sessionId];
	Player* player = playerMap_[oldPlayerId];

	playerMap_.erase(oldPlayerId);

	sessionIdToPlayerId_.insert({ sessionId, playerId });
	playerMap_.insert({ playerId, player });

	player->Initialize(sessionId, playerId, loginId, nickname);


	// 섹터 벡터에 삽입
	sector_[player->sectorX_][player->sectorY_].push_back(player);

	g_RpcProxy.ResLogin(sessionId, ELOGIN::SUCCESS, playerId);

	return true;
}

bool ChatServer::ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY)
{
	Player* player = SessionIdToPlayer(sessionId);

	if (player == nullptr)
		return false;

	sector_[player->sectorX_][player->sectorY_].remove(player);

	player->sectorX_ = sectorX;
	player->sectorY_ = sectorY;

	sector_[player->sectorX_][player->sectorY_].push_back(player);

	g_RpcProxy.ResSectorMove(sessionId, player->playerId_, player->sectorX_, player->sectorY_);
}

bool ChatServer::ReqMessage(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat)
{
	Player* player = SessionIdToPlayer(sessionId);

	if (player == nullptr)
		return false;

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

			for (auto& target : sector_[sectorNX][sectorNY])
			{
				//g_RpcProxy.ResMessage(target->sessionId_, player->playerId_, player->loginId_, player->nickname_, chat);
				sessionIds.push_back(target->sessionId_);
			}
		}
	}

	/// packet 재활용
	g_RpcProxy.ResMessage(sessionIds, player->playerId_, player->loginId_, player->nickname_, chat);

	return true;
}

bool ChatServer::ReqHeartbeat(uint64_t sessionId)
{
	Player* player = SessionIdToPlayer(sessionId);

	if (player == nullptr)
		return false;

	player->lastRecvTime_ = TickController::Instance().CurTime();

	return true;
}
