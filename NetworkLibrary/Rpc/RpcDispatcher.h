#pragma once
#include "../Utils/Packet.h"

class RpcStub;

class RpcDispatcher
{
public:

    bool DispatchPacket(uint64_t sessionId, Packet* packet, unsigned short type);

    RpcStub* stub_;
};
