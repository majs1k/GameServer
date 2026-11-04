#pragma once
#include <vector>
#include "../Utils/Packet.h"

class NetServer;

class RpcProxy
{
public:

    NetServer* server_;

public:

    void CS_ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey);
    void SC_ResLogin(uint64_t sessionId, byte status, int64_t playerId);
    void CS_ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY);
    void SC_ResSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY);
    void CS_ReqMessage(uint64_t sessionId, int64_t playerId, Chat& chat);
    void SC_ResMessage(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat);
    void CS_ReqHeartbeat(uint64_t sessionId);
    void CS_ReqLogin(std::vector<uint64_t>& sessionIds, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey);
    void SC_ResLogin(std::vector<uint64_t>& sessionIds, byte status, int64_t playerId);
    void CS_ReqSectorMove(std::vector<uint64_t>& sessionIds, int64_t playerId, ushort sectorX, ushort sectorY);
    void SC_ResSectorMove(std::vector<uint64_t>& sessionIds, int64_t playerId, ushort sectorX, ushort sectorY);
    void CS_ReqMessage(std::vector<uint64_t>& sessionIds, int64_t playerId, Chat& chat);
    void SC_ResMessage(std::vector<uint64_t>& sessionIds, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat);
    void CS_ReqHeartbeat(std::vector<uint64_t>& sessionIds);
};
