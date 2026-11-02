#include "TestServer.h"

#include "Utils/Packet.h"
#include "Utils/Logger.h"
#include "Utils/Profiler.h"
#include "Utils/TickController.h"
#include <Windows.h>

TestServer::TestServer()
{
	logicThread_ = std::thread(LogicThread, this);
}

TestServer::~TestServer()
{
	logicThread_.join();
}

bool TestServer::OnConnectionRequest(const std::string& ip, int port)
{
	if (sessionCount_ >= sessionMax_)
	{
		LOG(L"[Network] session limit over");

		return false;
	}

	return true;
}

void TestServer::OnAccept(uint64_t sessionId)
{
	//Packet* packet = new Packet();
	Packet* packet = packetPool_.Alloc();
	packet->Initialize(2, 1);

	uint64_t data = 0x7fffffffffffffff;

	*packet << data;

	this->SendPacket(sessionId, packet);
}

void TestServer::OnRelease(uint64_t sessionId)
{

}

void TestServer::OnRecv(uint64_t sessionId, Packet* packet)
{
	/// IO 스레드에서 패킷 처리
	//uint64_t data;
	//*packet >> data;
	//
	//if (packet->IsError())
	//{
	//	Disconnect(sessionId);
	//	return;
	//}
	//
	////Packet* packet2 = new Packet();
	//Packet* packet2 = packetPool_.Alloc();
	//
	//packet2->Initialize(2, 1);
	//
	//*packet2 << data;
	//
	//this->SendPacket(sessionId, packet2);

	/// 로직 스레드에서 패킷 처리
	packet->AddRef();
	logicThreadMQ_.Enqueue({ sessionId, packet });
}

void TestServer::LogicThread(TestServer* server)
{
	while (!server->shutdown_)
	{
		server->PacketProc();

		server->Update();

		TickController::Instance().Update();
	}
}

void TestServer::PacketProc()
{
	while (1)
	{
		std::pair<uint64_t, Packet*> message;
		bool ret = logicThreadMQ_.Dequeue(message);

		if (ret == false)
			break;

		auto [sessionId, packet] = message;

		uint64_t data;
		*packet >> data;

		if (packet->IsError())
		{
			packet->Free();

			Disconnect(sessionId);
			
			continue;
		}

		packet->Free();

		Packet* packet2 = packetPool_.Alloc();
		packet2->Initialize(2, 1);

		*packet2 << data;

		this->SendPacket(sessionId, packet2);
	}
}

void TestServer::Update()
{

}
