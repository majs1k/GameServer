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

	struct Player
	{
		uint64_t sessionId_;
		int64_t playerId_;

		LoginId loginId_;
		Nickname nickname_;

		bool isLogin_;
		bool isInitSector_ ;
		DWORD lastRecvTime_;

		// 0 ~ 49
		unsigned short sectorX_;
		unsigned short sectorY_;

		//unsigned int lastChatTime_;
		//int curChatCount_;

		void Initialize(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname);
		void Initialize(uint64_t sessionId);
	};

private:

	std::thread logicThread_;
	LockFreeQueue<std::pair<uint64_t, Packet*>> messageQueue_;
	//LockQueue<std::pair<uint64_t, Packet*>> messageQueue_;

public:

	ChatServer();
	~ChatServer();

private:

	bool OnConnectionRequest(const std::string& ip, int port) override;
	void OnAccept(uint64_t sessionId) override;
	void OnRelease(uint64_t sessionId) override;
	void OnRecv(uint64_t sessionId, Packet* packet) override;

	static void LogicThread(ChatServer* server);

	void ProcessMessageQueue();
	void CheckTimeout();

	constexpr static int TIMEOUT_MS = 40'000;

private:

	// if(map[x] == ...) 로직 금지 (새 원소 만듦)
	std::unordered_map<uint64_t, Player*> sessionIdToPlayer_;
	std::unordered_map<int64_t, Player*> playerMap_;
	int playerCount_ = 0;


	bool CreatePlayer(uint64_t sessionId);
	bool RemovePlayer(uint64_t sessionId);

	
	constexpr static int SECTOR_WIDTH = 50;
	constexpr static int SECTOR_HEIGHT = 50;

	std::list<Player*> sector_[SECTOR_HEIGHT][SECTOR_WIDTH];

	constexpr static int dx[3] = { 0, 1, -1 };
	constexpr static int dy[3] = { 0 , 1, -1 };

	// ===================================================================================== //
	// RPC
	// ===================================================================================== //

private:

	bool ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey) override;

	bool ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY) override;

	bool ReqMessage(uint64_t sessionId, int64_t playerId, Chat& chat) override;

	bool ReqHeartbeat(uint64_t sessionId) override;
};
