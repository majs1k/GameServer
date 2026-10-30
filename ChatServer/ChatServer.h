#pragma once
#include <list>
#include <unordered_map>
#include <thread>

#include "Network/NetServer.h"
#include "Utils/Packet.h"
#include "Utils/LockFreeQueue.h"

#include "CommonProtocol.h"
#include "RPC/RpcStub.h"


class ChatServer : public NetServer, public RpcStub
{
private:

	constexpr static int TIMEOUT_MS = 40'000;

	struct Player
	{
		uint64_t sessionId_;
		int64_t playerId_;

		LoginId loginId_;
		Nickname nickname_;

		DWORD lastRecvTime_;

		unsigned short sectorX_;
		unsigned short sectorY_;

		//unsigned int lastChatTime_;
		//int curChatCount_;

		void Initialize(uint64_t sessionId, int64_t playerId, LoginId loginId, Nickname nickname);
		void Initialize(uint64_t sessionId, int64_t playerId);
	};

private:

	std::thread logicThread_;
	LockFreeQueue<std::pair<uint64_t, Packet*>> logicThreadMQ_;


public:

	ChatServer();
	~ChatServer();

private:

	bool OnConnectionRequest(const std::string& ip, int port) override;
	void OnAccept(uint64_t sessionId) override;
	void OnRelease(uint64_t sessionId) override;
	void OnRecv(uint64_t sessionId, Packet* packet) override;

	static void LogicThread(ChatServer* server);

	void ProcessMQ();
	void Update();


private:

	// if(map[x] == ...) 로직 금지 (새 원소 만듦)
	std::unordered_map<uint64_t, int64_t> sessionIdToPlayerId_;
	std::unordered_map<int64_t, Player*> playerMap_;
	int playerCount_ = 0;

	uint64_t playerIdSeed_ = 0;
	LockFreeQueue<uint64_t> disconnectedSessionId_;


	constexpr static int SECTOR_WIDTH = 50;
	constexpr static int SECTOR_HEIGHT = 50;

	std::list<Player*> sector_[SECTOR_HEIGHT][SECTOR_WIDTH];

	constexpr static int dx[3] = { 0, 1, -1 };
	constexpr static int dy[3] = { 0 , 1, -1 };

private:

	Player* SessionIdToPlayer(uint64_t sessionId);


	// ===================================================================================== //
	// RPC
	// ===================================================================================== //

private:

	bool ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey) override;

	bool ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY) override;

	bool ReqMessage(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat) override;

	bool ReqHeartbeat(uint64_t sessionId) override;
};
