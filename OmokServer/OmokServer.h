#pragma once
#include <thread>

#include "Network/LanServer.h"
#include "Utils/Packet.h"
#include "Utils/LockFreeQueue.h"

#include "ResponseCode.h"
#include "Player.h"
#include "MatchMaker.h"
#include "GameRoom.h"

#include "RPC/RpcStub.h"


class OmokServer : public LanServer, public RpcStub
{
public:

	OmokServer();
	~OmokServer();

private:

	bool OnConnectionRequest(const std::string& ip, int port) override;
	void OnAccept(uint64_t sessionId) override;
	void OnRelease(uint64_t sessionId) override;
	void OnRecv(uint64_t sessionId, Packet* packet) override;

	// ===================================================================================== //
	// Message
	// ===================================================================================== //
private:

	static void LogicThread(OmokServer* server);

	void ProcessMQ();
	void Update();

	// ===================================================================================== //
	// Contents
	// ===================================================================================== //
private:

	// 클라이언트에서 보내는 playerId는 신뢰할수 없음. 서버에서 sessionId를 매핑해서 알아냄
	std::unordered_map<__int64, Player*> sessionToPlayer_;
	std::unordered_map<int, Player*> playerMap_;
	int playerCount_ = 0;

	MatchMaker matchMaker_;
	GameRoomManager roomManager_;

	// ===================================================================================== //
	// RPC
	// ===================================================================================== //
private:

	bool ReqEnterLobby(__int64 sessionId) override;
	bool ReqChat(__int64 sessionId, std::string& message) override;

	bool ReqStartMatch(__int64 sessionId) override;
	bool ReqCancelMatch(__int64 sessionId) override;
	bool ReqLeaveRoom(__int64 sessionId) override;

	bool ReqPlaceStone(__int64 sessionId, short row, short col) override;

	// ===================================================================================== //
	// DB
	// ===================================================================================== //
private:


};
