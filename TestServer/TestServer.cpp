#include "TestServer.h"
#include "Utils/Packet.h"
#include "Utils/Logger.h"
#include "Utils/Profiler.h"
#include <Windows.h>

TestServer::TestServer()
{
	logicThread_ = std::thread(LogicThread, this);
	monitorThread_ = std::thread(monitorThread, this);
}

TestServer::~TestServer()
{
	logicThread_.join();
	monitorThread_.join();
}

bool TestServer::OnConnectionRequest(const std::string& ip, int port)
{
	return true;
}

void TestServer::OnAccept(uint64_t sessionId)
{
	//Packet* packet = new Packet();
	Packet* packet = packetPool_.Alloc();
	packet->Initialize(sizeof(LAN_HEADER));

	uint64_t data = 0x7fffffffffffffff;

	*packet << data;

	SendPacket(sessionId, packet);

	packet->Free();
}

void TestServer::OnRelease(uint64_t sessionId)
{

}

void TestServer::OnRecv(uint64_t sessionId, Packet* packet)
{
	/// IO 스레드에서 패킷 처리
	if (!PacketProc(sessionId, packet))
	{
		Disconnect(sessionId);
	}

	/// 로직 스레드에서 패킷 처리
	//Message* message = messagePool_.Alloc();
	////Message* message = new Message();

	//message->Initialize(sessionId, packet, 0);

	//packet->AddRef();
	//messageQueue_.Enqueue(message);
}

void TestServer::LogicThread(TestServer* server)
{
	while (!server->shutdown_)
	{
		server->ProcessMessageQueue();
	}
}

void TestServer::ProcessMessageQueue()
{
	while (1)
	{
		Message* message;
		bool ret = messageQueue_.Dequeue(message);

		if (ret == false)
			break;

		uint64_t sessionId = message->sessionId_;
		Packet* packet = message->packet_;
		ushort protocol = message->protocol_;

		if (!PacketProc(sessionId, packet))
		{
			Disconnect(sessionId);
		}

		if (packet != nullptr)
		{
			packet->Free();
		}

		messagePool_.Free(message);
	}
}

int test = 0;

bool TestServer::PacketProc(uint64_t sessionId, Packet* packet)
{
	uint64_t data;
	*packet >> data;

	test = data;

	if (packet->IsError())
		return false;

	Packet* packet2 = packetPool_.Alloc();
	packet2->Initialize(sizeof(LAN_HEADER));

	*packet2 << data;

	SendPacket(sessionId, packet2);

	packet2->Free();

	return true;
}

void TestServer::monitorThread(TestServer* server)
{
	while (!server->shutdown_)
	{
		server->Monitoring();

		Sleep(1'000);
	}
}

void TestServer::Monitoring()
{
	int acceptCount = acceptCount_.exchange(0);

	int recvMessageCount = recvMessageCount_.exchange(0);
	int sendMessageCount = sendMessageCount_.exchange(0);


	printf("=========================\n");
	printf("=========================\n\n");

	printf("Session	   : %d\n", sessionCount_);

	printf("Accept     : %d\n", acceptCountTotal_.load());
	printf("Discon     : %d\n", disconnectCountTotal_.load());

	printf("Accept TPS : %d\n", acceptCount);

	printf("MessageQ   : %d\n\n", messageQueue_.Size());

	printf("Recv   TPS : %d\n", recvMessageCount);
	printf("Send   TPS : %d\n\n", sendMessageCount);

	printf("Packet  Pool : %d\n", packetPool_.Capacity());
	printf("Message Pool : %d\n", messagePool_.Capacity());
}
