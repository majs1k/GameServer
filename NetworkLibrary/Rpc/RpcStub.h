#pragma once
#include "../Utils/Packet.h"

class RpcStub
{
public:

    bool PacketProc(uint64_t sessionId, Packet* packet);
    
    virtual bool ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey);
    virtual bool ResLogin(uint64_t sessionId, byte status, int64_t playerId);
    virtual bool ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY);
    virtual bool ResSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY);
    virtual bool ReqMessage(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat);
    virtual bool ResMessage(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat);
    virtual bool ReqHeartbeat(uint64_t sessionId);
};
