#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "OmokGame.h"

enum class PLAYER_STATE : short
{
	NONE,
	LOBBY,
	GAMEROOM,
};

class GameRoom;

// 클라이언트 보관용
struct PlayerInfo
{
	int playerId_;
	std::string playerName_;

	int rating_;

	PLAYER_STATE state_;

	STONE stone_ = STONE::NONE;

	PlayerInfo() = default;

	PlayerInfo(int playerId, const std::string& playerName, int rating, PLAYER_STATE state)
		: playerId_(playerId), playerName_(playerName), rating_(rating), state_(state)
	{
	}
};


struct Player
{
	__int64 sessionId_;

	int playerId_;
	std::string playerName_;

	int win_;
	int lose_;
	int rating_;

	int money_;

	PLAYER_STATE state_;

	int equippedInvenId_;

	GameRoom* gameRoom_;

	unsigned int lastChatTime_;
	int curChatCount_;

	STONE stone_;


	void Initialize(__int64 sessionId, int playerId, std::string& playerName, int win, int lose, int rating, int money, int equippedId)
	{
		sessionId_ = sessionId;

		playerId_ = playerId;
		playerName_ = playerName;

		win_ = win;
		lose_ = lose;
		rating_ = rating;

		money_ = money;

		state_ = PLAYER_STATE::NONE;
		equippedInvenId_ = equippedId;

		gameRoom_ = nullptr;

		lastChatTime_ = 0;
		curChatCount_ = 0;
	}

	// PlayerInfo 생성용
	PlayerInfo ToInfo() const
	{
		return PlayerInfo{ playerId_,playerName_,rating_,state_ };
	}
};
