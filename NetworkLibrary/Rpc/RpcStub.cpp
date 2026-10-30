#include "RpcStub.h"


bool RpcStub::PacketProc(uint64_t sessionId, Packet* packet)
{
    unsigned short type;
    *packet >> type;

    switch (type)
    {
        case 1:
        {
			int64_t playerId;
			LoginId loginId;
			Nickname nickname;
			SessionKey sessionKey;

            *packet >> playerId >> loginId >> nickname >> sessionKey;

            return ReqLogin(sessionId, playerId, loginId, nickname, sessionKey);
        }
        case 2:
        {
			byte status;
			int64_t playerId;

            *packet >> status >> playerId;

            return ResLogin(sessionId, status, playerId);
        }
        case 3:
        {
			int64_t playerId;
			ushort sectorX;
			ushort sectorY;

            *packet >> playerId >> sectorX >> sectorY;

            return ReqSectorMove(sessionId, playerId, sectorX, sectorY);
        }
        case 4:
        {
			int64_t playerId;
			ushort sectorX;
			ushort sectorY;

            *packet >> playerId >> sectorX >> sectorY;

            return ResSectorMove(sessionId, playerId, sectorX, sectorY);
        }
        case 5:
        {
			int64_t playerId;
			LoginId loginId;
			Nickname nickname;
			Chat chat;

            *packet >> playerId >> loginId >> nickname >> chat;

            return ReqMessage(sessionId, playerId, loginId, nickname, chat);
        }
        case 6:
        {
			int64_t playerId;
			LoginId loginId;
			Nickname nickname;
			Chat chat;

            *packet >> playerId >> loginId >> nickname >> chat;

            return ResMessage(sessionId, playerId, loginId, nickname, chat);
        }
        case 7:
        {

            *packet;

            return ReqHeartbeat(sessionId);
        }
    default:
    {
        // 로그 추가

        return false;
    }
    }
}


bool RpcStub::ReqLogin(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, SessionKey& sessionKey)
{
    return true;
}

bool RpcStub::ResLogin(uint64_t sessionId, byte status, int64_t playerId)
{
    return true;
}

bool RpcStub::ReqSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY)
{
    return true;
}

bool RpcStub::ResSectorMove(uint64_t sessionId, int64_t playerId, ushort sectorX, ushort sectorY)
{
    return true;
}

bool RpcStub::ReqMessage(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat)
{
    return true;
}

bool RpcStub::ResMessage(uint64_t sessionId, int64_t playerId, LoginId& loginId, Nickname& nickname, Chat& chat)
{
    return true;
}

bool RpcStub::ReqHeartbeat(uint64_t sessionId)
{
    return true;
}
