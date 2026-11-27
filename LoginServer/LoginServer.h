#pragma once
#include <thread>

#include "Network/NetServer.h"
#include "Utils/Packet.h"
#include "Utils/LockFreeQueue.h"
#include "Utils/MemoryPool.h"
#include "Database/MySQL.h"
#include "Database/Redis.h"

#include "CommonProtocol.h"
#include "RPC/RpcStub.h"


class LoginServer : public NetServer, public RpcStub
{
private:

	struct Message
	{
		uint64_t sessionId_;
		Packet* packet_;
		unsigned short protocol_;
		unsigned short type_;

		void Initialize(uint64_t sessionId, Packet* packet, unsigned short protocol, unsigned short type = 0);
	};

	enum PROTOCOL
	{
		CS_NETWORK_PACKET_LOGIN_SERVER = 100,
	};

public:

	LoginServer(std::string dbHost, std::string dbUser, std::string dbPassword, std::string dbSchema, std::string redisHost, int redisPort);
	~LoginServer();

	// ===================================================================================== //
	// NetServer virtual func
	// ===================================================================================== //
private:

	bool OnConnectionRequest(const std::string& ip, int port) override;
	void OnAccept(uint64_t sessionId) override;
	void OnRelease(uint64_t sessionId) override;
	void OnRecv(uint64_t sessionId, Packet* packet) override;

	// ===================================================================================== //
	// Thraed
	// ===================================================================================== //
private:

	static void LogicThread(LoginServer* server);
	static void MonitorThread(LoginServer* server);
	void LogicFunc();
	void Monitoring();

	constexpr static int LOGIC_THREAD_NUM = 1;
	constexpr static int LOGIC_CONCURRENT_COUNT = 1;
	std::vector<std::thread> logicThreads_;
	HANDLE logicCompletionPort_;
	std::thread monitorThread_;

	// ===================================================================================== //
	// Message & Player
	// ===================================================================================== //
private:

	MemoryPoolTls<Message> messagePool_;
	//LockFreeQueue<Message*> messageQueue_;
	std::atomic<int> messageCount_ = 0;

	// ===================================================================================== //
	// Protocol
	// ===================================================================================== //
private:

	void ProcessMessage(Message* message);

	constexpr static int TIMEOUT_MS = 40'000;
	std::atomic<int> procMessageCount_ = 0;

	// ===================================================================================== //
	// RPC
	// ===================================================================================== //
private:

	bool CS_ReqLogin(uint64_t sessionId, int64_t userId, SessionKey& sessionKey) override;

	// ===================================================================================== //
	// DB
	// ===================================================================================== //
public:

	void CreateTable();

private:

	MySQL mysql_;
	std::string dbHost_;
	std::string dbUser_;
	std::string dbPassword_;
	std::string dbSchema_;

	Redis redis_;
	std::string redisHost_;
	int redisPort_;
};
