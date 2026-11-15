#include "../Network/NetServer.h"
#include "RpcProxy.h"


void RpcProxy::CS_ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>(1);
    *packet << playerId << loginId << nickname << sessionKey;

    serverProxy_->SendPacket(sessionId, packet);

    packet->Free();
}

void RpcProxy::SC_ResLogin(uint64_t sessionId, byte status, int64_t playerId)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>(2);
    *packet << status << playerId;

    serverProxy_->SendPacket(sessionId, packet);

    packet->Free();
}

void RpcProxy::CS_ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>(3);
    *packet << playerId << sectorX << sectorY;

    serverProxy_->SendPacket(sessionId, packet);

    packet->Free();
}

void RpcProxy::SC_ResSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>(4);
    *packet << playerId << sectorX << sectorY;

    serverProxy_->SendPacket(sessionId, packet);

    packet->Free();
}

void RpcProxy::CS_ReqChat(uint64_t sessionId, int64_t playerId, Chat& chat)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>(5);
    *packet << playerId << chat;

    serverProxy_->SendPacket(sessionId, packet);

    packet->Free();
}

void RpcProxy::SC_ResChat(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>(6);
    *packet << playerId << loginId << nickname << chat;

    serverProxy_->SendPacket(sessionId, packet);

    packet->Free();
}

void RpcProxy::CS_ReqHeartbeat(uint64_t sessionId)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>(7);
    *packet;

    serverProxy_->SendPacket(sessionId, packet);

    packet->Free();
}


void RpcProxy::CS_ReqLogin(std::vector<uint64_t>& sessionIds, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>(1);
    *packet << playerId << loginId << nickname << sessionKey;

    for (auto& sessionId : sessionIds)
	{
		serverProxy_->SendPacket(sessionId, packet);
	}

    packet->Free();
}

void RpcProxy::SC_ResLogin(std::vector<uint64_t>& sessionIds, byte status, int64_t playerId)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>(2);
    *packet << status << playerId;

    for (auto& sessionId : sessionIds)
	{
		serverProxy_->SendPacket(sessionId, packet);
	}

    packet->Free();
}

void RpcProxy::CS_ReqSectorMove(std::vector<uint64_t>& sessionIds, int64_t playerId, ushort sectorX, ushort sectorY)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>(3);
    *packet << playerId << sectorX << sectorY;

    for (auto& sessionId : sessionIds)
	{
		serverProxy_->SendPacket(sessionId, packet);
	}

    packet->Free();
}

void RpcProxy::SC_ResSectorMove(std::vector<uint64_t>& sessionIds, int64_t playerId, ushort sectorX, ushort sectorY)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>(4);
    *packet << playerId << sectorX << sectorY;

    for (auto& sessionId : sessionIds)
	{
		serverProxy_->SendPacket(sessionId, packet);
	}

    packet->Free();
}

void RpcProxy::CS_ReqChat(std::vector<uint64_t>& sessionIds, int64_t playerId, Chat& chat)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>(5);
    *packet << playerId << chat;

    for (auto& sessionId : sessionIds)
	{
		serverProxy_->SendPacket(sessionId, packet);
	}

    packet->Free();
}

void RpcProxy::SC_ResChat(std::vector<uint64_t>& sessionIds, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>(6);
    *packet << playerId << loginId << nickname << chat;

    for (auto& sessionId : sessionIds)
	{
		serverProxy_->SendPacket(sessionId, packet);
	}

    packet->Free();
}

void RpcProxy::CS_ReqHeartbeat(std::vector<uint64_t>& sessionIds)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>(7);
    *packet;

    for (auto& sessionId : sessionIds)
	{
		serverProxy_->SendPacket(sessionId, packet);
	}

    packet->Free();
}
