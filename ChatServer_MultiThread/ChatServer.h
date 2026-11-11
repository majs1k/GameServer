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

		// 0 ~ 24
		//unsigned short mapX_;
		//unsigned short mapY_;

		void Initialize(int64_t playerId, LoginId& loginId, Nickname& nickname);
		void Initialize(uint64_t sessionId);
	};


	struct Message
	{
		uint64_t sessionId_;
		Packet* packet_;
		ushort protocol_;
		Player* player_;

		void Initialize(uint64_t sessionId, Packet* packet, ushort protocol, Player* player = nullptr)
		{
			sessionId_ = sessionId;
			packet_ = packet;
			protocol_ = protocol;
			player_ = player;
		}
	};

	enum PROTOCOL
	{
		CS_NETWORK_PACKET = 0,
		SS_CREATE_PLAYER = 1,
		SS_REMOVE_PLAYER = 2,
		SS_CHECK_TIMEOUT = 3,

		SS_SECTOR_MOVE	= 4,
		SS_CHAT			= 5,

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

	static void LogicThread(ChatServer* server, int mapId);
	static void SchedulerThread(ChatServer* server);
	static void MonitorThread(ChatServer* server);


	constexpr static int MAP_COUNT = 4;

	std::thread logicThread_[MAP_COUNT];
	std::thread schedulerThread_;
	std::thread monitorThread_;


	void Monitoring();

	constexpr static int TIMEOUT_MS = 40'000;

	// ===================================================================================== //
	// Message & Player
	// ===================================================================================== //

private:

	MemoryPoolTls<Message> messagePool_;

	//LockFreeQueue<Message*> messageQueue_;
	LockFreeQueue<Message*> messageQueue_[MAP_COUNT];

	inline static thread_local int tls_MapId_;

	MemoryPoolTls<Player> playerPool_;

	//std::unordered_map<uint64_t, Player*> sessionIdToPlayer_;
	//std::unordered_map<int64_t, Player*> playerMap_;
	inline static thread_local std::unordered_map<uint64_t, Player*> tls_SessionIdToPlayer_;
	inline static thread_local std::unordered_map<int64_t, Player*> tls_PlayerMap_;


	std::unordered_map<uint64_t, int> sessionIdToMapId_;
	std::mutex mapIdLock_;

	constexpr static int INITIAL_MAP_ID = 0;

	int playerCount_ = 0;


	constexpr static int SECTOR_WIDTH = 50;
	constexpr static int SECTOR_HEIGHT = 50;

	//std::list<Player*> sector_[SECTOR_HEIGHT][SECTOR_WIDTH];

	constexpr static int MAP_WIDTH = 25;
	constexpr static int MAP_HEIGHT = 25;

	inline static thread_local std::list<Player*> worldMap_[MAP_HEIGHT][MAP_WIDTH];



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


	// ===================================================================================== //
	// MultiThread
	// ===================================================================================== //

	void SS_ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY, Player* player);

	void SS_ReqChat(uint64_t sessionId, int64_t playerId, Chat& chat, std::vector<unsigned short>& sectorX, std::vector<unsigned short>& sectorY);


	int SectorToMapId(unsigned short x, unsigned short y);

	int SectorToMapX(unsigned short x);

	int SectorToMapY(unsigned short y);
};
