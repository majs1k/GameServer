#include "RpcDispatcher.h"
#include "RpcStub.h"


bool RpcDispatcher::DispatchPacket(uint64_t sessionId, Packet* packet, unsigned short type)
{
    switch (type)
    {
    case 1:
    {
        stub_->messageCount_CS_ReqEnterChatServer_++;

		int64_t playerId;
		Email email;
		Nickname nickname;
		SessionKey sessionKey;

        *packet >> playerId >> email >> nickname >> sessionKey;

        if (packet->IsError())
            return false;

        return stub_->CS_ReqEnterChatServer(sessionId, playerId, email, nickname, sessionKey);
    }
    case 2:
    {
        stub_->messageCount_SC_ResEnterChatServer_++;

		byte status;
		int64_t playerId;

        *packet >> status >> playerId;

        if (packet->IsError())
            return false;

        return stub_->SC_ResEnterChatServer(sessionId, status, playerId);
    }
    case 3:
    {
        stub_->messageCount_CS_ReqSectorMove_++;

		int64_t playerId;
		ushort sectorX;
		ushort sectorY;

        *packet >> playerId >> sectorX >> sectorY;

        if (packet->IsError())
            return false;

        return stub_->CS_ReqSectorMove(sessionId, playerId, sectorX, sectorY);
    }
    case 4:
    {
        stub_->messageCount_SC_ResSectorMove_++;

		int64_t playerId;
		ushort sectorX;
		ushort sectorY;

        *packet >> playerId >> sectorX >> sectorY;

        if (packet->IsError())
            return false;

        return stub_->SC_ResSectorMove(sessionId, playerId, sectorX, sectorY);
    }
    case 5:
    {
        stub_->messageCount_CS_ReqChat_++;

		int64_t playerId;
		Chat chat;

        *packet >> playerId >> chat;

        if (packet->IsError())
            return false;

        return stub_->CS_ReqChat(sessionId, playerId, chat);
    }
    case 6:
    {
        stub_->messageCount_SC_ResChat_++;

		int64_t playerId;
		Email email;
		Nickname nickname;
		Chat chat;

        *packet >> playerId >> email >> nickname >> chat;

        if (packet->IsError())
            return false;

        return stub_->SC_ResChat(sessionId, playerId, email, nickname, chat);
    }
    case 7:
    {
        stub_->messageCount_CS_ReqHeartbeat_++;


        *packet;

        if (packet->IsError())
            return false;

        return stub_->CS_ReqHeartbeat(sessionId);
    }
    case 101:
    {
        stub_->messageCount_CS_ReqLogin_++;

		int64_t userId;
		SessionKey sessionKey;

        *packet >> userId >> sessionKey;

        if (packet->IsError())
            return false;

        return stub_->CS_ReqLogin(sessionId, userId, sessionKey);
    }
    case 102:
    {
        stub_->messageCount_SC_ResLogin_++;

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

        return stub_->SC_ResLogin(sessionId, userId, status, email, nickname, gameServerIp, gameServerPort, chatServerIp, chatServerPort);
    }
    default:
    {
        return false;
    }
    }
}
