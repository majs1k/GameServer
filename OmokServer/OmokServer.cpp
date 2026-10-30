#include "OmokServer.h"
#include "Utils/Packet.h"
#include "Utils/Logger.h"
#include "Utils/TickController.h"

#include <Windows.h>


OmokServer::OmokServer()
{
	logicThread_ = std::thread(LogicThread, this);

	dbThread_ = std::thread(DatabaseThread, this);

	g_DbProxy.dbReqQueue_ = &dbReqQueue_;
	g_DbProxy.dbResQueue_ = &dbResQueue_;

	matchMaker_.Initialize(&roomManager_);

	roomManager_.SetPlayerManager(&playerManager_);
}

OmokServer::~OmokServer()
{
	logicThread_.join();
}

void OmokServer::AttachProxy(RpcServerProxy* rpcProxy)
{
	rpcProxy->server_ = this;
}

void OmokServer::AttachDb(Database* db)
{
	db_ = db;

	userRepository_.Initialize(db_);
	playerRepository_.Initialize(db_);
	inventoryRepository_.Initialize(db_);
}

bool OmokServer::OnConnectionRequest(const std::string& ip, int port)
{
	if (SessionCount() >= SessionMax())
	{
		LOG(L"[Network] session limit over");

		return false;
	}

	return true;
}

void OmokServer::OnAccept(uint64_t sessionId)
{

}

void OmokServer::OnRelease(uint64_t sessionId)
{
	Player* player = playerManager_.GetPlayerBySessionId(sessionId);

	if (player == nullptr)
		return;

	// 방에 있었다면 나가기
	if (player->gameRoom_)
		player->gameRoom_->LeaveRoom(player);

	int playerId = player->playerId_;

	playerManager_.RemovePlayer(player);

	// 다른 플레이어들에게 퇴장 알림
	for (auto& p : playerManager_.GetPlayers())
	{
		g_RpcProxy.ResLeaveLobby(p.second->sessionId_, playerId);
	}
}

void OmokServer::OnRecv(uint64_t sessionId, Packet* packet)
{
	// 로직 스레드에서 패킷 처리
	logicThreadMQ_.Enqueue({ sessionId, packet });
}

void OmokServer::LogicThread(OmokServer* server)
{
	while (!server->shutdown_)
	{
		server->ProcessMQ();

		server->ProcessDbResQueue();

		server->Update();

		TickController::Instance().Update();
	}
}

void OmokServer::ProcessMQ()
{
	while (1)
	{
		Message message;
		bool ret = logicThreadMQ_.Dequeue(message);

		if (ret == false)
			return;

		uint64_t sessionId = message.SessionId();
		Packet* packet = message.Packet();

		// 함수의 인자 자료형 주의
		if (!PacketProc(sessionId, packet))
		{
			//Disconnect(sessionId);

			return;
		}

		packet->Free();
	}
}

void OmokServer::Update()
{
	roomManager_.Update();
}


// ----------------------------------------------------- //

void OmokServer::DatabaseThread(OmokServer* server)
{
	while (!server->shutdown_)
	{
		server->ProcessDbReqQueue();

		//TODO: 삭제
		Sleep(10);
	}
}

void OmokServer::ProcessDbReqQueue()
{
	while (1)
	{
		Message message;

		bool ret = dbReqQueue_.Pop(message);

		if (ret == false)
			return;

		uint64_t sessionId = message.SessionId();
		Packet* packet = message.Packet();

		DbPacketProc(sessionId, packet);

		packet->Free();
	}
}

void OmokServer::ProcessDbResQueue()
{
	while (1)
	{
		Message message;

		bool ret = dbResQueue_.Pop(message);

		if (ret == false)
			return;

		uint64_t sessionId = message.SessionId();
		Packet* packet = message.Packet();

		DbPacketProc(sessionId, packet);

		packet->Free();
	}
}
