#include "ChatServer.h"
#include "RpcModule.h"
#include "RPC/RpcProxy.h"
#include "Utils/Logger.h"

#include <Windows.h>

void ChatServer::Player::Initialize(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname)
{
	sessionId_ = sessionId;
	playerId_ = playerId;

	loginId_ = loginId;
	nickname_ = nickname;

	isLogin_ = true;
	lastRecvTime_ = GetTickCount();

	//lastChatTime_ = 0;
	//curChatCount_ = 0;
}

void ChatServer::Player::Initialize(uint64_t sessionId)
{
	sessionId_ = sessionId;

	isLogin_ = false;
	isInitSector_ = false;
	lastRecvTime_ = GetTickCount();
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
	if (sessionCount_ >= sessionMax_)
	{
		LOG(L"[Network] session limit over");

		return false;
	}

	// TODO: 외부 ip당 접속자 제한 등..

	return true;
}

void ChatServer::OnAccept(uint64_t sessionId)
{
	Packet* packet = packetPool_.Alloc();
	packet->Initialize(5, 1);

	/// 타입 번호??
	*packet << static_cast<unsigned short>(100);

	messageQueue_.Enqueue({ sessionId, packet });
}

void ChatServer::OnRelease(uint64_t sessionId)
{
	Packet* packet = packetPool_.Alloc();
	packet->Initialize(5, 1);

	/// 타입 번호??
	*packet << static_cast<unsigned short>(101);

	messageQueue_.Enqueue({ sessionId, packet });
}


bool ChatServer::CreatePlayer(uint64_t sessionId)
{
	Player* player = new Player();

	player->Initialize(sessionId);

	sessionIdToPlayer_.insert({ player->sessionId_ , player });

	return true;
}

bool ChatServer::RemovePlayer(uint64_t sessionId)
{
	Player* player = sessionIdToPlayer_[sessionId];

	sessionIdToPlayer_.erase(sessionId);

	if (player->isLogin_)
		playerMap_.erase(player->playerId_);

	delete player;

	// TODO: 다른 플레이어들에게 퇴장 알림

	return true;
}


void ChatServer::OnRecv(uint64_t sessionId, Packet* packet)
{
	// 로직 스레드에서 패킷 처리
	packet->AddRef();
	messageQueue_.Enqueue({ sessionId, packet });
}

void ChatServer::LogicThread(ChatServer* server)
{
	while (!server->shutdown_)
	{
		server->ProcessMessageQueue();

		server->CheckTimeout();
	}
}

void ChatServer::ProcessMessageQueue()
{
	while (1)
	{
		std::pair<uint64_t, Packet*> message;
		bool ret = messageQueue_.Dequeue(message);

		if (ret == false)
			break;

		auto [sessionId, packet] = message;


		unsigned short type;
		*packet >> type;

		switch (type)
		{
		case 100:

			CreatePlayer(sessionId);
			break;

		case 101:

			RemovePlayer(sessionId);
			break;

		default:

			/// RPC 패킷 처리 (타입 자료형 주의)
			if (!PacketProc(sessionId, packet, type))
			{
				Disconnect(sessionId);
			}

			packet->Free();
		}
	}
}

void ChatServer::CheckTimeout()
{
	DWORD curTime = GetTickCount();

	// 타임아웃 플레이어 세션 종료
	for (auto& pm : playerMap_)
	{
		Player* p = pm.second;

		if (curTime - p->lastRecvTime_ > TIMEOUT_MS)
		{
			Disconnect(p->sessionId_);
		}
	}
}
