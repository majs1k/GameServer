#pragma once
#include <list>
#include <unordered_map>

#include "Network/NetServer.h"
#include "Utils/Packet.h"

#include "CommonProtocol.h"
#include "RPC/RpcStub.h"
#include "Rpc/RpcProxy.h"


class ChatServer_Multi : public NetServer, public RpcStub
{
private:

	struct Player
	{
		uint64_t sessionId_;
		int64_t playerId_;
		Email email_;
		Nickname nickname_;
		bool isLogin_;
		bool isInitSector_;
		// 0 ~ 49
		unsigned short sectorX_;
		unsigned short sectorY_;
		/// Ãß°¡
		int mapId_;

		void Initialize(int64_t playerId, Email& email, Nickname& nickname);
		void Initialize(uint64_t sessionId);
	};

	enum PROTOCOL
	{
		IN_CREATE_PLAYER = 1,
		IN_REMOVE_PLAYER = 2,

		IN_SECTOR_MOVE = 3,
		IN_CHAT = 4,
	};

public:

	ChatServer_Multi();
	~ChatServer_Multi();

private:

	bool OnConnectionRequest(const std::string& ip, int port) override;
	void OnAccept(uint64_t sessionId) override;
	void OnRelease(uint64_t sessionId) override;
	void OnRecv(uint64_t sessionId, Packet* packet) override;

	// ===================================================================================== //
	// Thraed
	// ===================================================================================== //
private:

	static void LogicThread(ChatServer_Multi* server, int mapId);
	static void MonitorThread(ChatServer_Multi* server);
	void Monitoring();

	constexpr static int MAP_COUNT = 4;
	constexpr static int INITIAL_MAP_ID = 0;
	inline static thread_local int tls_MapId_;

	std::thread logicThread_[MAP_COUNT];
	std::thread monitorThread_;

	// ===================================================================================== //
	// Player
	// ===================================================================================== //
private:

	MemoryPoolTls<Player> playerPool_;
	std::unordered_map<uint64_t, Player*> sessionIdToPlayer_;
	std::mutex sessionIdToPlayerLock_;
	std::unordered_map<int64_t, Player*> playerMap_;
	std::mutex playerMapLock_;
	int playerCount_ = 0;

	constexpr static int SECTOR_WIDTH = 50;
	constexpr static int SECTOR_HEIGHT = 50;
	constexpr static int MAP_WIDTH = 25;
	constexpr static int MAP_HEIGHT = 25;
	constexpr static int dx[3] = { 0, 1, -1 };
	constexpr static int dy[3] = { 0, 1, -1 };

	inline static thread_local std::list<Player*> worldMap_[MAP_HEIGHT][MAP_WIDTH];

	// ===================================================================================== //
	// Protocol
	// ===================================================================================== //
private:

	virtual bool OnRecvPacket(uint64_t sessionId, Packet* packet, ushort protocol)
	{
		switch (protocol)
		{
		case IN_CREATE_PLAYER:

			messageCount_SS_CreatePlayer_++;
			return IN_CreatePlayer(sessionId);
			break;

		case IN_REMOVE_PLAYER:

			messageCount_SS_RemovePlayer_++;
			return IN_RemovePlayer(sessionId);
			break;

		case IN_SECTOR_MOVE:

			return IN_ReqSectorMove(sessionId, packet);
			break;

		case IN_CHAT:

			return IN_ReqChat(sessionId, packet);
			break;

		default:

			__debugbreak();
		}
	}

	void CreatePlayer(uint64_t sessionId);
	void RemovePlayer(uint64_t sessionId);
	bool IN_CreatePlayer(uint64_t sessionId);
	bool IN_RemovePlayer(uint64_t sessionId);

	std::atomic<int> messageCount_CS_NETWORK_PACKET_ = 0;
	std::atomic<int> messageCount_SS_CreatePlayer_ = 0;
	std::atomic<int> messageCount_SS_RemovePlayer_ = 0;
	std::atomic<int> dupLoginCountTotal_ = 0;

	// ===================================================================================== //
	// RPC
	// ===================================================================================== //
private:

	bool CS_ReqEnterChatServer(uint64_t sessionId, int64_t playerId, Email& email, Nickname& nickname, SessionKey& sessionKey) override;
	bool CS_ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY) override;
	bool CS_ReqChat(uint64_t sessionId, int64_t playerId, Chat& chat) override;
	bool CS_ReqHeartbeat(uint64_t sessionId) override;

	RpcProxy proxy_;

	// ===================================================================================== //
	// MultiThread
	// ===================================================================================== //
private:

	int SectorToMapId(unsigned short x, unsigned short y);
	int SectorToMapX(unsigned short x);
	int SectorToMapY(unsigned short y);

	void CreatePkt_SS_ReqSectorMove(Message* message, uint64_t sessionId, int64_t playerId, unsigned short sectorX, unsigned short sectorY);
	void CreatePkt_SS_ReqChat(Message* message, uint64_t sessionId, int64_t playerId,
		Email email, Nickname nickname, Chat& chat, unsigned short sectorX, unsigned short sectorY);

	bool IN_ReqSectorMove(uint64_t sessionId, Packet* packet);
	bool IN_ReqChat(uint64_t sessionId, Packet* packet);
};
