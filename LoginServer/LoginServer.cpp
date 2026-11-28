#include "LoginServer.h"
#include "RpcModule.h"
#include "Utils/Logger.h"
#include "Utils/Profiler.h"


LoginServer::LoginServer()
{
	proxy_.owner_ = this;
	dispatcher_.stub_ = this;
	threadType_ = THREAD_TYPE::MULTI;

	CreateTable();

	monitorThread_ = std::thread(MonitorThread, this);
}

LoginServer::~LoginServer()
{
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
