#include "LoginServer.h"
#include "RpcModule.h"
#include "Utils/Logger.h"


void LoginServer::ProcessMessage(Message* message)
{
	uint64_t sessionId = message->sessionId_;
	Packet* packet = message->packet_;
	ushort protocol = message->protocol_;
	ushort type = message->type_;
	bool success;

	procMessageCount_++;

	switch (protocol)
	{
	case CS_NETWORK_PACKET_LOGIN_SERVER:
	{
		success = PacketProc(sessionId, packet, type);
		break;
	}

	default:

		__debugbreak();
	}

	if (!success)
	{
		Disconnect(sessionId);
	}

	if (packet != nullptr)
	{
		packet->Free();
	}

	messagePool_.Free(message);
}
