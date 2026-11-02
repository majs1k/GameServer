#pragma once
#include <thread>

#include "Network/LanServer.h"
#include "Utils/Packet.h"
#include "Utils/LockFreeQueue.h"


class TestServer : public LanServer
{
private:

	std::thread logicThread_;
	LockFreeQueue<std::pair<uint64_t, Packet*>> logicThreadMQ_;

public:

	TestServer();
	~TestServer();

private:

	bool OnConnectionRequest(const std::string& ip, int port) override;
	void OnAccept(uint64_t sessionId) override;
	void OnRelease(uint64_t sessionId) override;
	void OnRecv(uint64_t sessionId, Packet* packet) override;

	static void LogicThread(TestServer* server);

	void PacketProc();
	void Update();
};
