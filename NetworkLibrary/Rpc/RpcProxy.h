#pragma once
#include <vector>
#include "../Utils/Packet.h"

class NetServer;

class RpcProxy
{
public:
    NetServer* server_;

public:
    void ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey);
    void ResLogin(uint64_t sessionId, byte status, int64_t playerId);
    void ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY);
    void ResSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY);
    void ReqMessage(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat);
    void ResMessage(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat);
    void ReqHeartbeat(uint64_t sessionId);
    void ReqLogin(std::vector<uint64_t> sessionIds, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey);
    void ResLogin(std::vector<uint64_t> sessionIds, byte status, int64_t playerId);
    void ReqSectorMove(std::vector<uint64_t> sessionIds, int64_t playerId, ushort sectorX, ushort sectorY);
    void ResSectorMove(std::vector<uint64_t> sessionIds, int64_t playerId, ushort sectorX, ushort sectorY);
    void ReqMessage(std::vector<uint64_t> sessionIds, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat);
    void ResMessage(std::vector<uint64_t> sessionIds, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat);
    void ReqHeartbeat(std::vector<uint64_t> sessionIds);
};
