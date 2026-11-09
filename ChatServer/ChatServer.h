#pragma once
#include <list>
#include <unordered_map>
#include <thread>

#include "Network/NetServer.h"
#include "Utils/Packet.h"
#include "Utils/LockFreeQueue.h"
#include "Utils/MemoryPool.h"

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
		uint64_t lastRecvTime_;

		// 0 ~ 49
		unsigned short sectorX_;
		unsigned short sectorY_;

		//unsigned int lastChatTime_;
		//int curChatCount_;

		void Initialize(int64_t playerId, LoginId& loginId, Nickname& nickname);
		void Initialize(uint64_t sessionId);
	};


	struct Message
	{
		uint64_t sessionId_;
		Packet* packet_;
		ushort protocol_;

		void Initialize(uint64_t sessionId, Packet* packet, ushort protocol)
		{
			sessionId_ = sessionId;
			packet_ = packet;
			protocol_ = protocol;
		}
	};

	enum PROTOCOL
	{
		CS_NETWORK_PACKET = 0,
		SS_CREATE_PLAYER = 1,
		SS_REMOVE_PLAYER = 2,
		SS_CHECK_TIMEOUT = 3,
	};

public:

	ChatServer();
	~ChatServer();

private:

	bool OnConnectionRequest(const std::string& ip, int port) override;
	void OnAccept(uint64_t sessionId) override;
	void OnRelease(uint64_t sessionId) override;
	void OnRecv(uint64_t sessionId, Packet* packet) override;

private:

	static void LogicThread(ChatServer* server);
	static void SchedulerThread(ChatServer* server);
	static void MonitorThread(ChatServer* server);

	std::thread logicThread_;
	std::thread schedulerThread_;
	std::thread monitorThread_;


	void Monitoring();

	constexpr static int TIMEOUT_MS = 40'000;

	// ===================================================================================== //
	// Message & Player
	// ===================================================================================== //

private:

	MemoryPoolTls<Message> messagePool_;

	LockFreeQueue<Message*> messageQueue_;

	//MemoryPoolTls<Player> playerPool_;
	MemoryPool<Player> playerPool_;

	// if(map[x] == ...) 로직 금지 (새 원소 만듦)
	std::unordered_map<uint64_t, Player*> sessionIdToPlayer_;
	std::unordered_map<int64_t, Player*> playerMap_;
	int playerCount_ = 0;


	constexpr static int SECTOR_WIDTH = 50;
	constexpr static int SECTOR_HEIGHT = 50;

	std::list<Player*> sector_[SECTOR_HEIGHT][SECTOR_WIDTH];

	constexpr static int dx[3] = { 0, 1, -1 };
	constexpr static int dy[3] = { 0, 1, -1 };

	// ===================================================================================== //
	// Protocol
	// ===================================================================================== //

private:

	void ProcessMessageQueue();

	void CreatePlayer(uint64_t sessionId);
	void RemovePlayer(uint64_t sessionId);
	void CheckTimeout();

	void SS_CreatePlayer(uint64_t sessionId);
	void SS_RemovePlayer(uint64_t sessionId);
	void SS_CheckTimeout(uint64_t sessionId);


	std::atomic<int> procMessageCount_ = 0;

	std::atomic<int> messageCount_SS_CreatePlayer_ = 0;
	std::atomic<int> messageCount_SS_RemovePlayer_ = 0;


	// ===================================================================================== //
	// RPC
	// ===================================================================================== //

private:

	bool CS_ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey) override;
	bool CS_ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY) override;
	bool CS_ReqChat(uint64_t sessionId, int64_t playerId, Chat& chat) override;
	bool CS_ReqHeartbeat(uint64_t sessionId) override;


	std::atomic<int> messageCount_CS_NETWORK_PACKET_ = 0;

	std::atomic<int> messageCount_CS_ReqLogin_ = 0;
	std::atomic<int> messageCount_CS_ReqSectorMove_ = 0;
	std::atomic<int> messageCount_CS_ReqChat_ = 0;


	std::atomic<int> dupLoginCountTotal_ = 0;
};
