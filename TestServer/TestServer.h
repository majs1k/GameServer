#pragma once
#include <thread>

#include "Network/LanServer.h"
#include "Utils/Packet.h"
#include "Utils/LockFreeQueue.h"
#include "Utils/LockQueue.h"


class TestServer : public LanServer
{
private:

	struct Message
	{
		uint64_t sessionId_;
		Packet* packet_;
		ushort protocol_;

	public:

		void Initialize(uint64_t sessionId, Packet* packet, ushort protocol)
		{
			sessionId_ = sessionId;
			packet_ = packet;
			protocol_ = protocol;
		}
	};

public:

	TestServer();
	~TestServer();

private:

	bool OnConnectionRequest(const std::string& ip, int port) override;
	void OnAccept(uint64_t sessionId) override;
	void OnRelease(uint64_t sessionId) override;
	void OnRecv(uint64_t sessionId, Packet* packet) override;

	static void LogicThread(TestServer* server);
	void ProcessMessageQueue();
	bool PacketProc(uint64_t sessionId, Packet* packet);

	std::thread logicThread_;
	LockFreeQueue<Message*> messageQueue_;
	MemoryPoolTls<Message> messagePool_;

	std::thread monitorThread_;
	static void monitorThread(TestServer* server);
	void Monitoring();
};
