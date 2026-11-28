#pragma once
#include <list>
#include <unordered_map>

#include "Network/NetServer.h"
#include "Utils/Packet.h"
#include "Utils/MemoryPool.h"

#include "CommonProtocol.h"
#include "RPC/RpcStub.h"
#include "Rpc/RpcProxy.h"


class ChatServer : public NetServer, public RpcStub
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

		void Initialize(int64_t playerId, Email& email, Nickname& nickname);
		void Initialize(uint64_t sessionId);
	};

public:

	ChatServer();
	~ChatServer();

private:

	bool OnConnectionRequest(const std::string& ip, int port) override;
	void OnAccept(uint64_t sessionId) override;
	void OnRelease(uint64_t sessionId) override;
	void OnRecv(uint64_t sessionId, Packet* packet) override;

	// ===================================================================================== //
	// Thraed
	// ===================================================================================== //
private:

	static void MonitorThread(ChatServer* server);
	void Monitoring();

	std::thread monitorThread_;

	// ===================================================================================== //
	// Player
	// ===================================================================================== //
private:

	MemoryPool<Player> playerPool_;
	std::unordered_map<uint64_t, Player*> sessionIdToPlayer_;
	std::unordered_map<int64_t, Player*> playerMap_;
	int playerCount_ = 0;

	constexpr static int SECTOR_WIDTH = 50;
	constexpr static int SECTOR_HEIGHT = 50;
	constexpr static int dx[3] = { 0, 1, -1 };
	constexpr static int dy[3] = { 0, 1, -1 };

	std::list<Player*> sector_[SECTOR_HEIGHT][SECTOR_WIDTH];

	// ===================================================================================== //
	// Protocol
	// ===================================================================================== //
private:

	enum PROTOCOL
	{
		IN_CREATE_PLAYER = 1,
		IN_REMOVE_PLAYER = 2,
	};

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

		default:

			__debugbreak();
		}
	}

	void CreatePlayer(uint64_t sessionId);
	void RemovePlayer(uint64_t sessionId);
	bool IN_CreatePlayer(uint64_t sessionId);
	bool IN_RemovePlayer(uint64_t sessionId);

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

	/// 새로 정의된 타입을 알아야해서 여기서 정의
	RpcProxy proxy_;
};
