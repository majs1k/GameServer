#include "LoginServer.h"
#include "RpcModule.h"
#include "Utils/Logger.h"
#include "Utils/Profiler.h"


void LoginServer::Message::Initialize(uint64_t sessionId, Packet* packet, unsigned short protocol, unsigned short type)
{
	sessionId_ = sessionId;
	packet_ = packet;
	protocol_ = protocol;
	type_ = type;
}

LoginServer::LoginServer(std::string dbHost, std::string dbUser, std::string dbPassword, std::string dbSchema, std::string redisHost, int redisPort)
{
	dbHost_ = dbHost;
	dbUser_ = dbUser;
	dbPassword_ = dbPassword;
	dbSchema_ = dbSchema;

	mysql_.Connect(dbHost_, dbUser_, dbPassword_, dbSchema_);

	CreateTable();

	redisHost_ = redisHost;
	redisPort_ = redisPort;

	/// Redis 연결
	//redis_.Connect(redisHost, redisPort);

	g_RpcProxy.serverProxy_ = this;

	for (int i = 0; i < LOGIC_THREAD_NUM; i++)
	{
		logicThreads_.emplace_back(std::thread(LogicThread, this));
	}

	logicCompletionPort_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, LOGIC_CONCURRENT_COUNT);

	monitorThread_ = std::thread(MonitorThread, this);
}

LoginServer::~LoginServer()
{
	for (int i = 0; i < LOGIC_THREAD_NUM; i++)
	{
		logicThreads_[i].join();
	}

	monitorThread_.join();
}

bool LoginServer::OnConnectionRequest(const std::string& ip, int port)
{
	return true;
}

void LoginServer::OnAccept(uint64_t sessionId)
{
}

void LoginServer::OnRelease(uint64_t sessionId)
{
}

void LoginServer::OnRecv(uint64_t sessionId, Packet* packet)
{
	unsigned short type;
	*packet >> type;

	Message* message = messagePool_.Alloc();
	message->Initialize(sessionId, packet, CS_NETWORK_PACKET_LOGIN_SERVER, type);
	packet->AddRef();

	messageCount_++;
	PostQueuedCompletionStatus(logicCompletionPort_, 0, (ULONG_PTR)message, nullptr);
}

void LoginServer::LogicThread(LoginServer* server)
{
	server->LogicFunc();
}

void LoginServer::LogicFunc()
{
	DWORD numOfBytes;
	Message* message;
	OVERLAPPED* overlapped_;

	while (1)
	{
		//session = nullptr;
		//numOfBytes = 0;

		// GQCS 호출 반환시 overlapped 구조체 무조건 세팅
		BOOL ret = GetQueuedCompletionStatus(logicCompletionPort_, &numOfBytes, reinterpret_cast<PULONG_PTR>(&message), &overlapped_, INFINITE);

		if (overlapped_ == nullptr && message == nullptr && numOfBytes == 0)
		{
			LOG("NETWORK", ELOG::SYSTEM_LEVEL, "logic Thread Stop");
			return;
		}

		if (!ret)
		{
			__debugbreak();
		}

		messageCount_--;
		ProcessMessage(message);
	}
}

void LoginServer::MonitorThread(LoginServer* server)
{
	while (!server->shutdown_)
	{
		server->Monitoring();

		Sleep(1'000);
	}
}

void LoginServer::Monitoring()
{
	int acceptCount = acceptCount_.exchange(0);
	int procMessgaeCount = procMessageCount_.exchange(0);
	int recvMessageCount = recvMessageCount_.exchange(0);
	int sendMessageCount = sendMessageCount_.exchange(0);

	printf("=========================\n");
	printf("LOGIN SERVER\n");
	printf("=========================\n\n");

	printf("Session	   : %d\n", sessionCount_);

	printf("Accept     : %d\n", acceptCountTotal_.load());
	printf("Discon     : %d\n", disconnectCountTotal_.load());
	printf("Timeout    : %d\n\n", timeoutCountTotal_.load());

	printf("Accept TPS : %d\n", acceptCount);
	printf("MessageCnt : %d\n", messageCount_.load());
	printf("SendWork   : %d\n", sendWorkCount_.load());
	printf("Update TPS : %d\n", procMessgaeCount);
	printf("Recv   TPS : %d\n", recvMessageCount);
	printf("Send   TPS : %d\n\n", sendMessageCount);

	printf("Packet  Pool : %d\n", packetPool_.Capacity());
	printf("Message Pool : %d\n\n", messagePool_.Capacity());
}

void LoginServer::CreateTable()
{
	//mysql_.Execute(
	//	"DROP TABLE users;"
	//);

	mysql_.Execute(
		"CREATE TABLE IF NOT EXISTS users ("
		"user_id BIGINT PRIMARY KEY,"
		"email VARCHAR(100) NOT NULL UNIQUE,"
		"nickname VARCHAR(100) NOT NULL UNIQUE,"
		"created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
		"updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP "
		"ON UPDATE CURRENT_TIMESTAMP,"
		"last_login_at TIMESTAMP"
		")"
	);
}
