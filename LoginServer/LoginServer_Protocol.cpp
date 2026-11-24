#include "LoginServer.h"
#include "Utils/Logger.h"

void LoginServer::ProcessMessage(Message* message)
{
	uint64_t sessionId = message->sessionId_;
	Packet* packet = message->packet_;
	ushort protocol = message->protocol_;
	bool isSuccess;

	procMessageCount_++;

	switch (protocol)
	{
	case CS_NETWORK_PACKET_LOGIN_SERVER:
	{
		messageCount_CS_NETWORK_PACKET_++;
		isSuccess = PacketProc(sessionId, packet);
		break;
	}

	default:

		/// ¼öÁ¤
		__debugbreak();
	}

	if (!isSuccess)
	{
		Disconnect(sessionId);
	}

	//auto it = sessionIdToPlayer_.find(sessionId);

	//if (it != sessionIdToPlayer_.end())
	//{
	//	Player* player = it->second;
	//	player->lastRecvTime_ = GetTickCount64();
	//}

	if (packet != nullptr)
	{
		packet->Free();
	}

	messagePool_.Free(message);
}
