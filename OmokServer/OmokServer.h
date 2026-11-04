#pragma once
#include <thread>

#include "Network/LanServer.h"
#include "Utils/Packet.h"
#include "Utils/LockFreeQueue.h"

#include "ResponseCode.h"
#include "Player.h"
#include "MatchMaker.h"
#include "GameRoom.h"

#include "Database/Database.h"

#include "Repository/UserRepository.h"
#include "Repository/PlayerRepository.h"
#include "Repository/InventoryRepository.h"

//#include "RPC/RpcServerProxy.h"
#include "RPC/RpcStub.h"

//#include "Database/DbProxy.h"
#include "Database/DbStub.h"


class RpcServerProxy;
class DbProxy;


class OmokServer : public LanServer, public RpcStub, public DbStub
{
private:

	std::thread logicThread_;
	LockFreeQueue<Message> logicThreadMQ_;

public:

	OmokServer();
	~OmokServer();

private:

	bool OnConnectionRequest(const std::string& ip, int port) override;
	void OnAccept(uint64_t sessionId) override;
	void OnRelease(uint64_t sessionId) override;
	void OnRecv(uint64_t sessionId, Packet* packet) override;

	static void LogicThread(OmokServer* server);

	void ProcessMQ();
	void Update();


	// ===================================================================================== //

private:

	Database db_;

	LockFreeQueue<Message> dbReqQueue_;

	std::thread dbThread_;
	static void DatabaseThread(OmokServer* server);


private:

	void ProcessDbReqQueue();


	// ===================================================================================== //
	// Repository
	// ===================================================================================== //

private:

	UserRepository userRepository_;
	PlayerRepository playerRepository_;
	InventoryRepository inventoryRepository_;


	// ===================================================================================== //
	// Contents
	// ===================================================================================== //

private:

	// 클라이언트에서 보내는 playerId는 신뢰할수 없음. 서버에서 sessionId를 매핑해서 알아냄
	//std::unordered_map<__int64, int> sessionToPlayer_;
	//std::unordered_map<int, Player*> playerMap_{};
	//int playerCount_ = 0;

	PlayerManager playerManager_;

	MatchMaker matchMaker_;

	GameRoomManager roomManager_;

	// ===================================================================================== //
	// RPC
	// ===================================================================================== //

private:

	bool ReqUserRegister(__int64 sessionId, std::string& loginId, std::string& password) override;

	bool ReqUserLogin(__int64 sessionId, std::string& loginId, std::string& password) override;

	bool ReqPlayerRegister(__int64 sessionId, int userId, std::string& playerName) override;

	bool ReqPlayerConnection(__int64 sessionId, int userId) override;

	bool ReqEnterLobby(__int64 sessionId) override;


	bool ReqChat(__int64 sessionId, std::string& message) override;

	bool ReqStartMatch(__int64 sessionId) override;

	bool ReqCancelMatch(__int64 sessionId) override;


	bool ReqPlaceStone(__int64 sessionId, short row, short col) override;

	bool ReqLeaveRoom(__int64 sessionId) override;


	// ===================================================================================== //
	// DB
	// ===================================================================================== //

private:

	bool ReqUserRegisterDB(__int64 sessionId, std::string& loginId, std::string& password) override;

	bool ResUserRegisterDB(__int64 sessionId, RESPONSE_CODE code) override;

	bool ReqUserLoginDB(__int64 sessionId, std::string& loginId, std::string& password) override;

	bool ResUserLoginDB(__int64 sessionId, RESPONSE_CODE code, int userId) override;

	bool ReqPlayerRegisterDB(__int64 sessionId, int userId, std::string& playerName) override;

	bool ResPlayerRegisterDB(__int64 sessionId, RESPONSE_CODE code) override;


	bool ReqPlayerConnectionDB(__int64 sessionId, int userId) override;

	bool ResPlayerProfileDB(__int64 sessionId, Player& player) override;

	bool ReqChangeEquipmentDB(__int64 sessionId, int playerId, int inventoryId) override;

	bool ReqGameResultDB(__int64 sessionId, int winnerId, int loserId) override;
};
