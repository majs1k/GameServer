#include "../Network/NetServer.h"
#include "RpcProxy.h"


void RpcProxy::ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(5, 1);

    *packet << 1;
    *packet << playerId << loginId << nickname << sessionKey;

    server_->SendPacket(sessionId, packet);
}

void RpcProxy::ResLogin(uint64_t sessionId, byte status, int64_t playerId)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(5, 1);

    *packet << 2;
    *packet << status << playerId;

    server_->SendPacket(sessionId, packet);
}

void RpcProxy::ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(5, 1);

    *packet << 3;
    *packet << playerId << sectorX << sectorY;

    server_->SendPacket(sessionId, packet);
}

void RpcProxy::ResSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(5, 1);

    *packet << 4;
    *packet << playerId << sectorX << sectorY;

    server_->SendPacket(sessionId, packet);
}

void RpcProxy::ReqMessage(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(5, 1);

    *packet << 5;
    *packet << playerId << loginId << nickname << chat;

    server_->SendPacket(sessionId, packet);
}

void RpcProxy::ResMessage(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(5, 1);

    *packet << 6;
    *packet << playerId << loginId << nickname << chat;

    server_->SendPacket(sessionId, packet);
}

void RpcProxy::ReqHeartbeat(uint64_t sessionId)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(5, 1);

    *packet << 7;
    *packet;

    server_->SendPacket(sessionId, packet);
}


void RpcProxy::ReqLogin(std::vector<uint64_t> sessionIds, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(5, sessionIds.size());

    *packet << 1;
    *packet << playerId << loginId << nickname << sessionKey;

    for (auto& sessionId : sessionIds)
	{
		server_->SendPacket(sessionId, packet);
	}
}

void RpcProxy::ResLogin(std::vector<uint64_t> sessionIds, byte status, int64_t playerId)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(5, sessionIds.size());

    *packet << 2;
    *packet << status << playerId;

    for (auto& sessionId : sessionIds)
	{
		server_->SendPacket(sessionId, packet);
	}
}

void RpcProxy::ReqSectorMove(std::vector<uint64_t> sessionIds, int64_t playerId, ushort sectorX, ushort sectorY)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(5, sessionIds.size());

    *packet << 3;
    *packet << playerId << sectorX << sectorY;

    for (auto& sessionId : sessionIds)
	{
		server_->SendPacket(sessionId, packet);
	}
}

void RpcProxy::ResSectorMove(std::vector<uint64_t> sessionIds, int64_t playerId, ushort sectorX, ushort sectorY)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(5, sessionIds.size());

    *packet << 4;
    *packet << playerId << sectorX << sectorY;

    for (auto& sessionId : sessionIds)
	{
		server_->SendPacket(sessionId, packet);
	}
}

void RpcProxy::ReqMessage(std::vector<uint64_t> sessionIds, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(5, sessionIds.size());

    *packet << 5;
    *packet << playerId << loginId << nickname << chat;

    for (auto& sessionId : sessionIds)
	{
		server_->SendPacket(sessionId, packet);
	}
}

void RpcProxy::ResMessage(std::vector<uint64_t> sessionIds, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(5, sessionIds.size());

    *packet << 6;
    *packet << playerId << loginId << nickname << chat;

    for (auto& sessionId : sessionIds)
	{
		server_->SendPacket(sessionId, packet);
	}
}

void RpcProxy::ReqHeartbeat(std::vector<uint64_t> sessionIds)
{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(5, sessionIds.size());

    *packet << 7;
    *packet;

    for (auto& sessionId : sessionIds)
	{
		server_->SendPacket(sessionId, packet);
	}
}
