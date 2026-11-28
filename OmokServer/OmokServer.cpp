#include "OmokServer.h"
#include "Utils/Packet.h"
#include "Utils/Logger.h"
#include "Utils/TickController.h"

#include <Windows.h>


OmokServer::OmokServer()
{
	logicThread_ = std::thread(LogicThread, this);

	matchMaker_.Initialize(&roomManager_);

	g_RpcProxy->server_ = this;


	auto stmt = database_->Prepare(
		"CREATE TABLE IF NOT EXISTS players("
		"player_id INT AUTO_INCREMENT PRIMARY KEY,"
		"user_id INT NOT NULL UNIQUE,"
		"player_name VARCHAR(30) NOT NULL UNIQUE,"
		"money INT NOT NULL DEFAULT 10000,"
		"win INT NOT NULL DEFAULT 0,"
		"lose INT NOT NULL DEFAULT 0,"
		"rating INT NOT NULL DEFAULT 1000,"
		"profile_icon_id INT NOT NULL DEFAULT 0"
		");");

	stmt->execute();
}

OmokServer::~OmokServer()
{
	logicThread_.join();
}

bool OmokServer::OnConnectionRequest(const std::string& ip, int port)
{
	return true;
}

void OmokServer::OnAccept(uint64_t sessionId)
{
}

void OmokServer::OnRelease(uint64_t sessionId)
{
	Player* player = sessionId;

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
}

void OmokServer::LogicThread(OmokServer* server)
{
	while (!server->shutdown_)
	{
		server->ProcessMQ();

		server->Update();

		TickController::Instance().Update();
	}
}

void OmokServer::Update()
{
	roomManager_.Update();
}
