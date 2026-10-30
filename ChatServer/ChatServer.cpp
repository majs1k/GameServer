#include "ChatServer.h"
#include "RpcModule.h"
#include "RPC/RpcProxy.h"
#include "Utils/Logger.h"
#include "Utils/TickController.h"

#include <Windows.h>


void ChatServer::Player::Initialize(uint64_t sessionId, int64_t playerId, LoginId loginId, Nickname nickname)
{
	sessionId_ = sessionId;
	playerId_ = playerId;

	loginId_ = loginId;
	nickname_ = nickname;

	// Session에 위치하면 동기화 문제 발생
	lastRecvTime_ = TickController::Instance().CurTime();

	// TODO: 0으로 초기화??
	sectorX_ = 0;
	sectorY_ = 0;

	//lastChatTime_ = 0;
	//curChatCount_ = 0;
}

void ChatServer::Player::Initialize(uint64_t sessionId, int64_t playerId)
{
	sessionId_ = sessionId;
	playerId_ = playerId;

	lastRecvTime_ = TickController::Instance().CurTime();
}


ChatServer::ChatServer()
{
	logicThread_ = std::thread(LogicThread, this);

	g_RpcProxy.server_ = this;
}

ChatServer::~ChatServer()
{
	logicThread_.join();
}

bool ChatServer::OnConnectionRequest(const std::string& ip, int port)
{
	if (SessionCount() >= SessionMax())
	{
		LOG(L"[Network] session limit over");

		return false;
	}

	// TODO: 외부 ip당 접속자 제한 등..

	return true;
}

void ChatServer::OnAccept(uint64_t sessionId)
{
	Player* player = new Player();

	/// playerId를 음수로 초기화
	player->Initialize(sessionId, --playerIdSeed_);

	sessionIdToPlayerId_.insert({ player->sessionId_ , player->playerId_ });
	playerMap_.insert({ player->playerId_, player });
}

void ChatServer::OnRelease(uint64_t sessionId)
{
	/// 로직스레드에서 컨텐츠 삭제
	disconnectedSessionId_.Enqueue(sessionId);

	// 다른 플레이어들에게 퇴장 알림

	return;
}

void ChatServer::OnRecv(uint64_t sessionId, Packet* packet)
{
	// 로직 스레드에서 패킷 처리
	logicThreadMQ_.Enqueue({ sessionId, packet });
}

void ChatServer::LogicThread(ChatServer* server)
{
	while (!server->shutdown_)
	{
		server->ProcessMQ();

		server->Update();

		TickController::Instance().Update();
	}
}

void ChatServer::ProcessMQ()
{
	DWORD curTime = TickController::Instance().CurTime();

	while (1)
	{
		std::pair<uint64_t, Packet*> message;
		bool ret = logicThreadMQ_.Dequeue(message);

		if (ret == false)
			return;

		auto [sessionId, packet] = message;

		Player* player = SessionIdToPlayer(sessionId);

		if (player == nullptr)
		{
			Disconnect(sessionId);

			return;
		}

		/// 로직스레드에서 타임아웃 갱신
		player->lastRecvTime_ = curTime;

		// 함수의 인자 자료형 주의
		//if (!PacketProc(sessionId, packet))
		//{
		//	Disconnect(sessionId);

		//	return;
		//}

		packet->Free();
	}
}

void ChatServer::Update()
{
	/// 세션 종료된 플레이어 삭제 (여기서만 삭제)
	while (1)
	{
		uint64_t sessionId;

		bool ret = disconnectedSessionId_.Dequeue(sessionId);

		if (ret == false)
			break;

		if (sessionIdToPlayerId_.count(sessionId) == 0)
			return;

		int64_t playerId = sessionIdToPlayerId_[sessionId];

		if (playerMap_.count(playerId) == 0)
		{
			sessionIdToPlayerId_.erase(sessionId);

			return;
		}

		Player* player = playerMap_[playerId];

		sessionIdToPlayerId_.erase(sessionId);
		playerMap_.erase(playerId);

		delete player;
	}

	DWORD curTime = TickController::Instance().CurTime();

	/// 타임아웃 플레이어 세션 종료
	for (auto& pm : playerMap_)
	{
		Player* p = pm.second;

		if (curTime - p->lastRecvTime_ > TIMEOUT_MS)
		{
			Disconnect(p->sessionId_);
		}
	}
}

ChatServer::Player* ChatServer::SessionIdToPlayer(uint64_t sessionId)
{
	if (sessionIdToPlayerId_.count(sessionId) == 0)
		return nullptr;

	int64_t playerId = sessionIdToPlayerId_[sessionId];

	if (playerMap_.count(playerId) == 0)
	{
		sessionIdToPlayerId_.erase(sessionId);

		return nullptr;
	}

	return playerMap_[playerId];
}
