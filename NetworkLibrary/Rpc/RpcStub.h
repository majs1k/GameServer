#pragma once
#include <atomic>

class RpcStub
{
    friend class RpcDispatcher;

public:

    virtual ~RpcStub() = default;

    virtual bool CS_ReqEnterChatServer(uint64_t sessionId, int64_t playerId, Email& email, Nickname& nickname, SessionKey& sessionKey);
    virtual bool SC_ResEnterChatServer(uint64_t sessionId, byte status, int64_t playerId);
    virtual bool CS_ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY);
    virtual bool SC_ResSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY);
    virtual bool CS_ReqChat(uint64_t sessionId, int64_t playerId, Chat& chat);
    virtual bool SC_ResChat(uint64_t sessionId, int64_t playerId, Email& email, Nickname& nickname, Chat& chat);
    virtual bool CS_ReqHeartbeat(uint64_t sessionId);
    virtual bool CS_ReqLogin(uint64_t sessionId, int64_t userId, SessionKey& sessionKey);
    virtual bool SC_ResLogin(uint64_t sessionId, int64_t userId, byte status, Email& email, Nickname& nickname, IpAddress& gameServerIp, ushort gameServerPort, IpAddress& chatServerIp, ushort chatServerPort);

protected:

    std::atomic<int> messageCount_CS_ReqEnterChatServer_{0};
    std::atomic<int> messageCount_SC_ResEnterChatServer_{0};
    std::atomic<int> messageCount_CS_ReqSectorMove_{0};
    std::atomic<int> messageCount_SC_ResSectorMove_{0};
    std::atomic<int> messageCount_CS_ReqChat_{0};
    std::atomic<int> messageCount_SC_ResChat_{0};
    std::atomic<int> messageCount_CS_ReqHeartbeat_{0};
    std::atomic<int> messageCount_CS_ReqLogin_{0};
    std::atomic<int> messageCount_SC_ResLogin_{0};
};
