#include "ChatServer.h"
#include "RpcModule.h"
#include "Utils/Logger.h"
#include "Utils/Profiler.h"


void ChatServer::Player::Initialize(int64_t playerId, Email& email, Nickname& nickname)
{
	playerId_ = playerId;
	email_ = email;
	nickname_ = nickname;
	isLogin_ = true;
	//isInitSector_ = false;
}

void ChatServer::Player::Initialize(uint64_t sessionId)
{
	sessionId_ = sessionId;
	isLogin_ = false;
	isInitSector_ = false;
}

void ChatServer::Message::Initialize(uint64_t sessionId, Packet* packet, unsigned short protocol, unsigned short type)
{
	sessionId_ = sessionId;
	packet_ = packet;
	protocol_ = protocol;
	type_ = type;
}

ChatServer::ChatServer()
{
	g_RpcProxy.serverProxy_ = this;

	logicThread_ = std::thread(LogicThread, this);
	monitorThread_ = std::thread(MonitorThread, this);
}

ChatServer::~ChatServer()
{
	logicThread_.join();
	monitorThread_.join();
}

bool ChatServer::OnConnectionRequest(const std::string& ip, int port)
{
	return true;
}

void ChatServer::OnAccept(uint64_t sessionId)
{
	CreatePlayer(sessionId);
}

void ChatServer::OnRelease(uint64_t sessionId)
{
	RemovePlayer(sessionId);
}

void ChatServer::OnRecv(uint64_t sessionId, Packet* packet)
{
	// 로직 스레드에서 패킷 처리
	// IO 스레드에서 처리하도록 수정 x (컨텐츠에서 동기화 문제 발생 가능)
	unsigned short type;
	*packet >> type;
	Message* message = messagePool_.Alloc();
	message->Initialize(sessionId, packet, CS_NETWORK_PACKET_CHAT_SERVER, type);
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

void ChatServer::MonitorThread(ChatServer* server)
{
	while (!server->shutdown_)
	{
		server->Monitoring();

		Sleep(1'000);
	}
}

void ChatServer::Monitoring()
{
	int acceptCount = acceptCount_.exchange(0);
	int procMessgaeCount = procMessageCount_.exchange(0);
	int recvMessageCount = recvMessageCount_.exchange(0);
	int sendMessageCount = sendMessageCount_.exchange(0);

	int messageCount_SS_CreatePlayer = messageCount_SS_CreatePlayer_.exchange(0);
	int messageCount_SS_RemovePlayer = messageCount_SS_RemovePlayer_.exchange(0);
	int messageCount_NETWORK_PACKET = messageCount_CS_NETWORK_PACKET_.exchange(0);
	int messageCount_CS_ReqConnectChatServer = messageCount_CS_ReqEnterChatServer_.exchange(0);
	int messageCount_CS_ReqSectorMove = messageCount_CS_ReqSectorMove_.exchange(0);
	int messageCount_CS_ReqChat = messageCount_CS_ReqChat_.exchange(0);

	printf("=========================\n");
	printf("CHAT SERVER\n");
	printf("=========================\n\n");

	printf("Session	   : %d\n", sessionCount_);
	printf("Player	   : %d\n\n", playerCount_);

	printf("Accept     : %d\n", acceptCountTotal_.load());
	printf("Discon     : %d\n", disconnectCountTotal_.load());
	printf("DupLogin   : %d\n", dupLoginCountTotal_.load());
	printf("Timeout    : %d\n", timeoutCountTotal_.load());
	printf("Accept TPS : %d\n", acceptCount);
	printf("MessageQ   : %d\n", messageQueue_.Size());
	printf("SendWork   : %d\n\n", sendWorkCount_.load());

	printf("Update TPS : %d\n", procMessgaeCount);
	printf("Recv   TPS : %d\n", recvMessageCount);
	printf("Send   TPS : %d\n", sendMessageCount);
	printf("Create     : %d\n", messageCount_SS_CreatePlayer);
	printf("Remove     : %d\n", messageCount_SS_RemovePlayer);
	printf("Network    : %d\n", messageCount_NETWORK_PACKET);
	printf("Connect    : %d\n", messageCount_CS_ReqConnectChatServer);
	printf("Move       : %d\n", messageCount_CS_ReqSectorMove);
	printf("Chat       : %d\n\n", messageCount_CS_ReqChat);

	printf("Packet  Pool : %d\n", packetPool_.Capacity());
	printf("Message Pool : %d\n", messagePool_.Capacity());
	printf("Player  Pool : %d\n\n", playerPool_.Capacity());
}
