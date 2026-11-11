#include "RpcStub.h"


bool RpcStub::PacketProc(uint64_t sessionId, Packet* packet)
{
    unsigned short type;
    *packet >> type;

    switch (type)
    {
    case 1:
    {
        messageCount_CS_ReqLogin_++;

		int64_t playerId;
		LoginId loginId;
		Nickname nickname;
		SessionKey sessionKey;

        *packet >> playerId >> loginId >> nickname >> sessionKey;
    
        return CS_ReqLogin(sessionId, playerId, loginId, nickname, sessionKey);
    }
    case 2:
    {
		byte status;
		int64_t playerId;

        *packet >> status >> playerId;
    
        return SC_ResLogin(sessionId, status, playerId);
    }
    case 3:
    {
        messageCount_CS_ReqSectorMove_++;

		int64_t playerId;
		ushort sectorX;
		ushort sectorY;

        *packet >> playerId >> sectorX >> sectorY;
    
        return CS_ReqSectorMove(sessionId, playerId, sectorX, sectorY);
    }
    case 4:
    {
		int64_t playerId;
		ushort sectorX;
		ushort sectorY;

        *packet >> playerId >> sectorX >> sectorY;
    
        return SC_ResSectorMove(sessionId, playerId, sectorX, sectorY);
    }
    case 5:
    {
        messageCount_CS_ReqChat_++;

		int64_t playerId;
		Chat chat;

        *packet >> playerId >> chat;
    
        return CS_ReqChat(sessionId, playerId, chat);
    }
    case 6:
    {
		int64_t playerId;
		LoginId loginId;
		Nickname nickname;
		Chat chat;

        *packet >> playerId >> loginId >> nickname >> chat;
    
        return SC_ResChat(sessionId, playerId, loginId, nickname, chat);
    }
    case 7:
    {

        *packet;
    
        return CS_ReqHeartbeat(sessionId);
    }
    default:
    {
        return false;
    }
    }

    if (packet != nullptr)
    {
        if (packet->IsError())
        {
            __debugbreak();
        }
    }
}


bool RpcStub::CS_ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey)
{
    return true;
}

bool RpcStub::SC_ResLogin(uint64_t sessionId, byte status, int64_t playerId)
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

bool RpcStub::SC_ResChat(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat)
{
    return true;
}

bool RpcStub::CS_ReqHeartbeat(uint64_t sessionId)
{
    return true;
}
