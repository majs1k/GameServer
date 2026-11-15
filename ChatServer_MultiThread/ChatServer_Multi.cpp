#include "ChatServer.h"
#include "RpcModule.h"


void ChatServer::SS_ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY)
{
	Player* player;

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		auto it = sessionIdToPlayer_.find(sessionId);

		if (it == sessionIdToPlayer_.end())
		{
			return;
		}

		player = it->second;

		/// TODO: 이건 안전한 코드일까?
		player->mapId_ = tls_MapId_;
	}

	/// TODO: 안전하게 수정?
	tls_PlayerMap_[sessionId] = player;


	if (sectorX < 0 || sectorX >= 50 || sectorY < 0 || sectorY >= 50)
		__debugbreak();

	player->sectorX_ = sectorX;
	player->sectorY_ = sectorY;

	worldMap_[SectorToMapY(player->sectorY_)][SectorToMapX(player->sectorX_)].push_back(player);

	g_RpcProxy.SC_ResSectorMove(sessionId, player->playerId_, player->sectorX_, player->sectorY_);
}

void ChatServer::SS_ReqChat(uint64_t sessionId, int64_t playerId, Chat& chat, std::vector<unsigned short>& sectorX, std::vector<unsigned short>& sectorY)
{
	Player* player;

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		auto it = sessionIdToPlayer_.find(sessionId);

		if (it == sessionIdToPlayer_.end())
		{
			return;
		}

		player = it->second;
	}

	if (!player->isLogin_)
		return;

	if (!player->isInitSector_)
	{
		return;
	}

	if (playerId != player->playerId_)
	{
		//__debugbreak();
		//LOG(L"reqmessage playerid error");

		return;
	}

	std::vector<uint64_t> sessionIds;

	for (int i = 0; i < sectorX.size(); i++)
	{
		for (auto& target : worldMap_[SectorToMapY(sectorY[i])][SectorToMapX(sectorX[i])])
		{
			//g_RpcProxy.ResMessage(target->sessionId_, player->playerId_, player->loginId_, player->nickname_, chat);
			if (target->isLogin_)
			{
				sessionIds.push_back(target->sessionId_);
			}
		}
	}

	/// packet 재활용
	g_RpcProxy.SC_ResChat(sessionIds, player->playerId_, player->loginId_, player->nickname_, chat);
}


int ChatServer::SectorToMapId(unsigned short x, unsigned short y)
{
	if (0 <= x && x < 25 && 0 <= y && y < 25)
		return  0;
	else if (25 <= x && x < 50 && 0 <= y && y < 25)
		return 1;
	else if (0 <= x && x < 25 && 25 <= y && y < 50)
		return 2;
	else if (25 <= x && x < 50 && 25 <= y && y < 50)
		return 3;
	else
		return -1;
}

int ChatServer::SectorToMapX(unsigned short x)
{
	return x % MAP_WIDTH;
}

int ChatServer::SectorToMapY(unsigned short y)
{
	return y % MAP_HEIGHT;
}
