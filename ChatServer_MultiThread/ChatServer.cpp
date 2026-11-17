#include "ChatServer_Multi.h"
#include "RpcModule.h"
#include "Utils/Logger.h"
#include "Utils/Profiler.h"


void ChatServer_Multi::Player::Initialize(int64_t playerId, LoginId& loginId, Nickname& nickname)
{
	playerId_ = playerId;
	loginId_ = loginId;
	nickname_ = nickname;
	isLogin_ = true;
	//isInitSector_ = false;
}

void ChatServer_Multi::Player::Initialize(uint64_t sessionId)
{
	sessionId_ = sessionId;
	isLogin_ = false;
	isInitSector_ = false;
}

void ChatServer_Multi::Message::Initialize(uint64_t sessionId, Packet* packet, ushort protocol)
{
	sessionId_ = sessionId;
	packet_ = packet;
	protocol_ = protocol;
}

ChatServer_Multi::ChatServer_Multi()
{
	g_RpcProxy.serverProxy_ = this;

	for (int i = 0; i < MAP_COUNT; i++)
	{
		logicThread_[i] = std::thread(LogicThread, this, i);
	}

	schedulerThread_ = std::thread(SchedulerThread, this);
	monitorThread_ = std::thread(MonitorThread, this);
}

ChatServer_Multi::~ChatServer_Multi()
{
	for (int i = 0; i < MAP_COUNT; i++)
	{
		logicThread_[i].join();
	}

	schedulerThread_.join();
	monitorThread_.join();
}

bool ChatServer_Multi::OnConnectionRequest(const std::string& ip, int port)
{
	return true;
}

void ChatServer_Multi::OnAccept(uint64_t sessionId)
{
	CreatePlayer(sessionId);
}

void ChatServer_Multi::OnRelease(uint64_t sessionId)
{
	RemovePlayer(sessionId);
}

void ChatServer_Multi::OnRecv(uint64_t sessionId, Packet* packet)
{
	Player* player;

	{
		std::lock_guard<std::mutex> lock(sessionIdToPlayerLock_);

		auto it = sessionIdToPlayer_.find(sessionId);

		if (it == sessionIdToPlayer_.end())
			return;

		player = it->second;
	}

	Message* message = messagePool_.Alloc();
	message->Initialize(sessionId, packet, CS_NETWORK_PACKET);
	packet->AddRef();

	messageQueue_[player->mapId_].Enqueue(message);
}

void ChatServer_Multi::LogicThread(ChatServer_Multi* server, int mapId)
{
	tls_MapId_ = mapId;

	while (!server->shutdown_)
	{
		server->ProcessMessageQueue();
	}
}

void ChatServer_Multi::SchedulerThread(ChatServer_Multi* server)
{
	int64_t cnt = 0;

	while (!server->shutdown_)
	{
		if (cnt % 10 == 0)
		{
			server->CheckTimeout();
		}

		cnt++;

		Sleep(1'000);
	}
}

void ChatServer_Multi::MonitorThread(ChatServer_Multi* server)
{
	while (!server->shutdown_)
	{
		server->Monitoring();

		Sleep(1'000);
	}
}


void ChatServer_Multi::Monitoring()
{
	int acceptCount = acceptCount_.exchange(0);

	int procMessgaeCount = procMessageCount_.exchange(0);
	int recvMessageCount = recvMessageCount_.exchange(0);
	int sendMessageCount = sendMessageCount_.exchange(0);

	int messageCount_SS_CreatePlayer = messageCount_SS_CreatePlayer_.exchange(0);
	int messageCount_SS_RemovePlayer = messageCount_SS_RemovePlayer_.exchange(0);
	int messageCount_NETWORK_PACKET = messageCount_CS_NETWORK_PACKET_.exchange(0);

	int messageCount_CS_ReqLogin = messageCount_CS_ReqLogin_.exchange(0);
	int messageCount_CS_ReqSectorMove = messageCount_CS_ReqSectorMove_.exchange(0);
	int messageCount_CS_ReqChat = messageCount_CS_ReqChat_.exchange(0);


	printf("=========================\n");
	printf("=========================\n\n");

	printf("Session	   : %d\n", sessionCount_);
	printf("Player	   : %d\n\n", playerCount_);

	printf("Accept     : %d\n", acceptCountTotal_.load());
	printf("Discon     : %d\n", disconnectCountTotal_.load());
	printf("DupLogin   : %d\n\n", dupLoginCountTotal_.load());

	printf("Accept TPS : %d\n", acceptCount);

	//printf("MessageQ   : %d\n\n", messageQueue_.Size());

	printf("Update TPS : %d\n", procMessgaeCount);
	printf("Recv   TPS : %d\n", recvMessageCount);
	printf("Send   TPS : %d\n\n", sendMessageCount);

	printf("Create     : %d\n", messageCount_SS_CreatePlayer);
	printf("Remove     : %d\n\n", messageCount_SS_RemovePlayer);

	printf("Network    : %d\n", messageCount_NETWORK_PACKET);
	printf("Login      : %d\n", messageCount_CS_ReqLogin);
	printf("Move       : %d\n", messageCount_CS_ReqSectorMove);
	printf("Chat       : %d\n\n", messageCount_CS_ReqChat);

	printf("Packet  Pool : %d\n", packetPool_.Capacity());
	printf("Message Pool : %d\n", messagePool_.Capacity());
	printf("Player  Pool : %d\n\n", playerPool_.Capacity());
}


int ChatServer_Multi::SectorToMapId(unsigned short x, unsigned short y)
{
	if (0 <= x && x < 25 && 0 <= y && y < 25)
		return  0;
	else if (25 <= x && x < 50 && 0 <= y && y < 25)
		return 1;
	else if (0 <= x && x < 25 && 25 <= y && y < 50)
		return 2;
	else if (25 <= x && x < 50 && 25 <= y && y < 50)
		return 3;
	else
		return -1;
}

int ChatServer_Multi::SectorToMapX(unsigned short x)
{
	return x % MAP_WIDTH;
}

int ChatServer_Multi::SectorToMapY(unsigned short y)
{
	return y % MAP_HEIGHT;
}
