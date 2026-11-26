#include "RpcStub.h"


bool RpcStub::PacketProc(uint64_t sessionId, Packet* packet)
{
    unsigned short type;
    *packet >> type;

    switch (type)
    {
    case 1:
    {
        messageCount_CS_ReqEnterChatServer_++;

		int64_t playerId;
		Email email;
		Nickname nickname;
		SessionKey sessionKey;

        *packet >> playerId >> email >> nickname >> sessionKey;

        if (packet->IsError())
            return false;

        return CS_ReqEnterChatServer(sessionId, playerId, email, nickname, sessionKey);
    }
    case 2:
    {
        messageCount_SC_ResEnterChatServer_++;

		byte status;
		int64_t playerId;

        *packet >> status >> playerId;

        if (packet->IsError())
            return false;

        return SC_ResEnterChatServer(sessionId, status, playerId);
    }
    case 3:
    {
        messageCount_CS_ReqSectorMove_++;

		int64_t playerId;
		ushort sectorX;
		ushort sectorY;

        *packet >> playerId >> sectorX >> sectorY;

        if (packet->IsError())
            return false;

        return CS_ReqSectorMove(sessionId, playerId, sectorX, sectorY);
    }
    case 4:
    {
        messageCount_SC_ResSectorMove_++;

		int64_t playerId;
		ushort sectorX;
		ushort sectorY;

        *packet >> playerId >> sectorX >> sectorY;

        if (packet->IsError())
            return false;

        return SC_ResSectorMove(sessionId, playerId, sectorX, sectorY);
    }
    case 5:
    {
        messageCount_CS_ReqChat_++;

		int64_t playerId;
		Chat chat;

        *packet >> playerId >> chat;

        if (packet->IsError())
            return false;

        return CS_ReqChat(sessionId, playerId, chat);
    }
    case 6:
    {
        messageCount_SC_ResChat_++;

		int64_t playerId;
		Email email;
		Nickname nickname;
		Chat chat;

        *packet >> playerId >> email >> nickname >> chat;

        if (packet->IsError())
            return false;

        return SC_ResChat(sessionId, playerId, email, nickname, chat);
    }
    case 7:
    {
        messageCount_CS_ReqHeartbeat_++;


        *packet;

        if (packet->IsError())
            return false;

        return CS_ReqHeartbeat(sessionId);
    }
    case 101:
    {
        messageCount_CS_ReqLogin_++;

		int64_t userId;
		SessionKey sessionKey;

        *packet >> userId >> sessionKey;

        if (packet->IsError())
            return false;

        return CS_ReqLogin(sessionId, userId, sessionKey);
    }
    case 102:
    {
        messageCount_SC_ResLogin_++;

		int64_t userId;
		byte status;
		Email email;
		Nickname nickname;
		IpAddress gameServerIp;
		ushort gameServerPort;
		IpAddress chatServerIp;
		ushort chatServerPort;

        *packet >> userId >> status >> email >> nickname >> gameServerIp >> gameServerPort >> chatServerIp >> chatServerPort;

        if (packet->IsError())
            return false;

        return SC_ResLogin(sessionId, userId, status, email, nickname, gameServerIp, gameServerPort, chatServerIp, chatServerPort);
    }
    default:
    {
        return false;
    }
    }
}


bool RpcStub::CS_ReqEnterChatServer(uint64_t sessionId, int64_t playerId, Email& email, Nickname& nickname, SessionKey& sessionKey)
{
    return true;
}

bool RpcStub::SC_ResEnterChatServer(uint64_t sessionId, byte status, int64_t playerId)
{
    return true;
}

bool RpcStub::CS_ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY)
{
    return true;
}

bool RpcStub::SC_ResSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY)
{
    return true;
}

bool RpcStub::CS_ReqChat(uint64_t sessionId, int64_t playerId, Chat& chat)
{
    return true;
}

bool RpcStub::SC_ResChat(uint64_t sessionId, int64_t playerId, Email& email, Nickname& nickname, Chat& chat)
{
    return true;
}

bool RpcStub::CS_ReqHeartbeat(uint64_t sessionId)
{
    return true;
}

bool RpcStub::CS_ReqLogin(uint64_t sessionId, int64_t userId, SessionKey& sessionKey)
{
    return true;
}

bool RpcStub::SC_ResLogin(uint64_t sessionId, int64_t userId, byte status, Email& email, Nickname& nickname, IpAddress& gameServerIp, ushort gameServerPort, IpAddress& chatServerIp, ushort chatServerPort)
{
    return true;
}
