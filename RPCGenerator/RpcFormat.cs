
using System;

namespace PacketGenerator
{
    class RpcFormat
    {
        public static string proxyHeader =
@"#pragma once
#include <vector>
#include ""../Utils/Packet.h""

class NetServer;

class RpcProxy
{{
public:
{0}

    NetServer* owner_;
}};
";

        public static string proxyHeaderFunc =
@"
    void {0}(uint64_t sessionId{1});";

        public static string proxyCppFunc =
@"
void RpcProxy::{0}(uint64_t sessionId{1})
{{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>({2});
    *packet{3};

    owner_->SendPacket(sessionId, packet);

    packet->Free();
}}
";

        public static string proxyCppHeader =
@"#include ""../Network/NetServer.h""
#include ""RpcProxy.h""
";

        public static string funcParam =
@"{0} {1}";

        public static string shiftLeft =
@" << {0}";

        public static string proxyHeaderFuncVector =
@"
    void {0}(std::vector<uint64_t>& sessionIds{1});";

        public static string proxyCppFuncVector =
@"
void RpcProxy::{0}(std::vector<uint64_t>& sessionIds{1})
{{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>({2});
    *packet{3};

    for (auto& sessionId : sessionIds)
    {{
        owner_->SendPacket(sessionId, packet);
    }}

    packet->Free();
}}
";

        public static string stubHeader =
@"#pragma once
#include <atomic>

class RpcStub
{{
    friend class RpcDispatcher;

public:

    virtual ~RpcStub() = default;
{0}

protected:
{1}
}};
";

        public static string stubHeaderFunc =
@"
    virtual bool {0}(uint64_t sessionId{1});";

        public static string stubHeaderMessageCount =
@"
    std::atomic<int> messageCount_{0}_{{0}};";

        public static string stubCppHeader =
@"#include ""RpcStub.h""
";

        public static string stubCppFunc =
@"
bool RpcStub::{0}(uint64_t sessionId{1})
{{
    return true;
}}
";

        public static string dispatcherHeader =
@"#pragma once
#include ""../Utils/Packet.h""

class RpcStub;

class RpcDispatcher
{{
public:

    bool DispatchPacket(uint64_t sessionId, Packet* packet, unsigned short type);

    RpcStub* stub_;
}};
";

        public static string dispatcherCppHeader =
@"#include ""RpcDispatcher.h""
#include ""RpcStub.h""
";

        public static string dispatcherCppDispatchPacket =
@"
bool RpcDispatcher::DispatchPacket(uint64_t sessionId, Packet* packet, unsigned short type)
{{
    switch (type)
    {{{0}
    default:
    {{
        return false;
    }}
    }}
}}
";

        public static string dispatcherDispatchPacketCase =
@"
    case {0}:
    {{
        stub_->messageCount_{3}_++;

{1}
        *packet{2};

        if (packet->IsError())
            return false;

        return stub_->{4}(sessionId{5});
    }}";

        public static string shiftRight =
@" >> {0}";
    }
}
