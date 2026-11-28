#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "OmokGame.h"

enum PLAYER_STATE
{
	NONE,
	LOBBY,
	GAMEROOM,
};

class GameRoom;

struct Player
{
	uint64_t sessionId_;
	int playerId_;
	std::string playerName_;

	int win_;
	int lose_;
	int rating_;

	byte state_;
	int money_;
	GameRoom* gameRoom_;

	unsigned int lastChatTime_;
	int curChatCount_;

	void Initialize(__int64 sessionId, int playerId, std::string& playerName, int win, int rating, int money, int equippedId)
	{
		sessionId_ = sessionId;
		playerId_ = playerId;
		playerName_ = playerName;
		money_ = money;
		state_ = PLAYER_STATE::NONE;

		gameRoom_ = nullptr;
		win_ = win;
		rating_ = rating;

		lastChatTime_ = 0;
		curChatCount_ = 0;
	}
};
