#include "OmokServer.h"
#include "RpcModule.h"


bool OmokServer::ReqEnterLobby(__int64 sessionId)
{
	Player* player = playerManager_.GetPlayerBySessionId(sessionId);

	if (player == nullptr)
		return true;

	player->state_ = PLAYER_STATE::LOBBY;

	// 플레이어에게 로비 플레이어들 정보 송신
	for (auto& p : playerManager_.GetPlayers())
	{
		PlayerInfo other = p.second->ToInfo();

		g_RpcProxy.ResEnterLobby(sessionId, other);
	}

	// 다른 플레이어들에게도 입장 알림
	for (auto& p : playerManager_.GetPlayers())
	{
		if (p.second->playerId_ == player->playerId_)
			continue;

		g_RpcProxy.ResEnterLobby(p.second->sessionId_, info);
	}

	return true;
}

bool OmokServer::ReqChat(__int64 sessionId, std::string& message)
{
	Player* player = playerManager_.GetPlayerBySessionId(sessionId);

	if (player == nullptr)
		return true;

	unsigned int now = GetTickCount();

	// 1초 내에 다시 채팅 보냈을시
	if (now - player->lastChatTime_ < 1000)
	{
		player->curChatCount_++;
	}

	// 채팅 도배 시 확인시점으로부터 5초간 금지
	if (now - player->lastChatTime_ >= 5000)
	{
		player->curChatCount_ = 0;
	}

	// 채팅 도배 금지
	if (player->curChatCount_ > 5)
	{
		std::string m = u8"잠시 후 다시 채팅해주세요.";
		g_RpcProxy.ResChat(sessionId, player->playerId_, m);

		return true;
	}

	player->lastChatTime_ = now;


	// 로비 내 사람들끼리 채팅
	if (player->state_ == PLAYER_STATE::LOBBY)
	{
		for (auto& i : playerManager_.GetPlayers())
		{
			if (i.second->state_ != PLAYER_STATE::LOBBY)
				continue;

			g_RpcProxy.ResChat(i.second->sessionId_, player->playerId_, message);
		}
	}
	// 게임룸 내 사람들끼리 채팅
	else if (player->state_ == PLAYER_STATE::GAMEROOM)
	{
		player->gameRoom_->BraodcastChat(player, message);
	}

	return true;
}

bool OmokServer::ReqStartMatch(__int64 sessionId)
{
	Player* player = playerManager_.GetPlayerBySessionId(sessionId);

	if (player == nullptr)
		return true;

	matchMaker_.Add(player);

	return true;
}

bool OmokServer::ReqCancelMatch(__int64 sessionId)
{
	Player* player = playerManager_.GetPlayerBySessionId(sessionId);

	if (player == nullptr)
		return true;

	matchMaker_.Remove(player);

	return true;
}

bool OmokServer::ReqPlaceStone(__int64 sessionId, short row, short col)
{
	Player* player = playerManager_.GetPlayerBySessionId(sessionId);

	if (player == nullptr)
		return true;

	player->gameRoom_->PlaceStone(player, row, col);

	return true;
}

bool OmokServer::ReqLeaveRoom(__int64 sessionId)
{
	Player* player = playerManager_.GetPlayerBySessionId(sessionId);

	if (player == nullptr)
		return true;

	int roomId = player->gameRoom_->GetRoomId();

	// 반환값이 true일 경우 gameroom 삭제
	if (player->gameRoom_->LeaveRoom(player))
		roomManager_.RemoveRoom(roomId);

	return true;
}
