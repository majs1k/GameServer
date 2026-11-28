#include "RpcStub.h"


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
