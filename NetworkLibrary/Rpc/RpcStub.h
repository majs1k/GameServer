#pragma once
#include "../Utils/Packet.h"

class RpcStub
{
public:

    bool PacketProc(uint64_t sessionId, Packet* packet);
    
    virtual bool CS_ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey);
    virtual bool SC_ResLogin(uint64_t sessionId, byte status, int64_t playerId);
    virtual bool CS_ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY);
    virtual bool SC_ResSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY);
    virtual bool CS_ReqChat(uint64_t sessionId, int64_t playerId, Chat& chat);
    virtual bool SC_ResChat(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat);
    virtual bool CS_ReqHeartbeat(uint64_t sessionId);
};
