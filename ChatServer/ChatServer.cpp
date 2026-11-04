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
	
	schedulerThread_ = std::thread(TimeoutThread, this);

	g_RpcProxy.server_ = this;
}

ChatServer::~ChatServer()
{
	logicThread_.join();

	schedulerThread_.join();
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
	CreatePlayer(sessionId);
}

void ChatServer::OnRelease(uint64_t sessionId)
{
	RemovePlayer(sessionId);

	test++;
}

void ChatServer::OnRecv(uint64_t sessionId, Packet* packet)
{
	// 로직 스레드에서 패킷 처리
	// IO 스레드에서 처리하도록 수정 x (컨텐츠에서 동기화 문제 발생 가능)

	//Message* message = messagePool_.Alloc();
	Message* message = new Message();

	message->Initialize(sessionId, packet, NETWORK_PACKET);

	packet->AddRef();

	messageQueue_.Enqueue(message);
}

void ChatServer::LogicThread(ChatServer* server)
{
	while (!server->shutdown_)
	{
		server->ProcessMessageQueue();
	}
}

void ChatServer::TimeoutThread(ChatServer* server)
{
	int64_t cnt = 0;

	while (!server->shutdown_)
	{
		server->Monitoring();

		if(cnt % 10 == 0)
			server->CheckTimeout();

		cnt++;

		Sleep(1'000);
	}
}


void ChatServer::CreatePlayer(uint64_t sessionId)
{
	//Message* message = messagePool_.Alloc();
	Message* message = new Message();

	message->Initialize(sessionId, nullptr, CREATE_PLAYER);

	messageQueue_.Enqueue(message);
}

void ChatServer::RemovePlayer(uint64_t sessionId)
{
	//Message* message = messagePool_.Alloc();
	Message* message = new Message();

	message->Initialize(sessionId, nullptr, REMOVE_PLAYER);

	messageQueue_.Enqueue(message);
}

void ChatServer::CheckTimeout()
{
	// 타임아웃 플레이어 세션 종료
	for (auto& pm : playerMap_)
	{
		Player* p = pm.second;

		int timeDiff = GetTickCount() - p->lastRecvTime_;

		if (timeDiff > TIMEOUT_MS)
		{
			//Message* message = messagePool_.Alloc();
			Message* message = new Message();

			message->Initialize(p->sessionId_, nullptr, CHECK_TIMEOUT);

			messageQueue_.Enqueue(message);
		}
	}
}

void ChatServer::Monitoring()
{
	//Message* message = messagePool_.Alloc();
	Message* message = new Message();

	message->Initialize(0, nullptr, MONITORING);

	messageQueue_.Enqueue(message);
}


void ChatServer::ProcessMessageQueue()
{
	while (1)
	{
		Message* message;
		bool ret = messageQueue_.Dequeue(message);

		if (ret == false)
			break;
		
		++procMessageCount_;

		uint64_t sessionId = message->sessionId_;
		Packet* packet = message->packet_;
		ushort protocol = message->protocol_;


		switch (protocol)
		{
		case NETWORK_PACKET:

			/// 네트워크 패킷 처리 (타입 자료형 주의)
			if (!PacketProc(sessionId, packet))
			{
				Disconnect(sessionId);
			}

			break;

		case CREATE_PLAYER:

			SS_CreatePlayer(sessionId);
			break;

		case REMOVE_PLAYER:

			SS_RemovePlayer(sessionId);
			break;

		case CHECK_TIMEOUT:

			SS_CheckTimeout(sessionId);
			break;

		case MONITORING:

			SS_Monitoring();
			break;

		default:

			__debugbreak();

		}

		if (packet != nullptr)
			packet->Free();


		delete message;
		//messagePool_.Free(message);
	}
}

void ChatServer::SS_CreatePlayer(uint64_t sessionId)
{
	//Player* player = playerPool_.Alloc();
	Player* player = new Player();

	player->Initialize(sessionId);

	sessionIdToPlayer_.insert({ player->sessionId_ , player });
}

void ChatServer::SS_RemovePlayer(uint64_t sessionId)
{
	Player* player = sessionIdToPlayer_[sessionId];

	sessionIdToPlayer_.erase(sessionId);

	// 무조건 이 함수에서 진행해야함
	if (player->isInitSector_)
	{
		sector_[player->sectorY_][player->sectorX_].remove(player);
	}

	if (player->isLogin_)
	{
		playerCount_--;

		playerMap_.erase({ player->playerId_ });

	}

	// TODO: 다른 플레이어들에게 퇴장 알림

	//playerPool_.Free(player);
	delete player;
}

void ChatServer::SS_CheckTimeout(uint64_t sessionId)
{
	Disconnect(sessionId);
}


void ChatServer::SS_Monitoring()
{
	int acceptCount = acceptCount_.exchange(0);
	int procMessgaeCount = procMessageCount_.exchange(0);
	int recvMessageCount = recvMessageCount_.exchange(0);
	int sendMessageCount = sendMessageCount_.exchange(0);


	printf("=====================\n");
	printf(" SERVER  MONITORING\n");
	printf("=====================\n\n");

	printf("Session Count : %d\n", sessionCount_);
	printf("Packet Pool Capa : %d\n\n", packetPool_.Capacity());

	printf("Message Pool Capa : %d\n", messagePool_.Capacity());
	printf("Message Queue Size : %d\n\n", messageQueue_.Size());


	printf("Player Pool Capa : %d\n", playerPool_.Capacity());
	printf("Player Count : %d\n\n", playerCount_);

	printf("Accept Total : %d\n", acceptCountTotal_);
	printf("Accept TPS : %d\n", acceptCount);
	printf("Proc Message TPS : %d\n\n", procMessgaeCount);

	printf("Recv Message TPS : %d\n", recvMessageCount);
	printf("Send Message TPS : %d\n\n", sendMessageCount);
}
