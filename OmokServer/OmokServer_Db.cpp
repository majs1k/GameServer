#include "OmokServer.h"
#include "RpcModule.h"


bool OmokServer::ReqPlayerConnection(__int64 sessionId, int userId)
{
	auto stmt = mysql_.Execute(
		"SELECT player_id, player_name, win, rating, money "
		"FROM players "
		"WHERE user_id = ?");

	/// + db에 없을시 플레이어 생성하라 보내주기

	p.playerId_ = result->getInt("player_id");
	p.playerName_ = result->getString("player_name");
	p.win_ = result->getInt("win");
	p.rating_ = result->getInt("rating");
	p.money_ = result->getInt("money");

	return true;
}

bool OmokServer::ReqGameResult(int winnerId)
{
	mysql_.Execute(
		"UPDATE players SET win = win + 1, rating = rating + 1, money = mone + 1 "
		"WHERE player_id = " + std::to_string(winnerId));

	return true;
}
