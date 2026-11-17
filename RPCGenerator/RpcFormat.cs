using System;

namespace PacketGenerator
{
    class RpcFormat
    {
        // ============================================================
        // Proxy
        // ============================================================

        public static string proxyHeader =
@"#pragma once
#include <vector>
#include ""../Utils/Packet.h""

class NetServer;

class RpcProxy
{{
public:

    NetServer* serverProxy_;

public:
{0}
}};
";

        // Header에는 함수 선언만
        public static string proxyHeaderFunc =
@"
    void {0}(uint64_t sessionId{1});";

        // CPP에 들어갈 실제 구현
        public static string proxyCppFunc =
@"
void RpcProxy::{0}(uint64_t sessionId{1})
{{
    Packet* packet = packetPool_.Alloc();
    packet->Initialize(sizeof(NET_HEADER));

    *packet << static_cast<unsigned short>({2});
    *packet{3};

    serverProxy_->SendPacket(sessionId, packet);

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



        // 벡터 버전 함수 선언
        public static string proxyHeaderFuncVector =
        @"
    void {0}(std::vector<uint64_t>& sessionIds{1});";


        // 벡터 버전 함수 구현
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
		serverProxy_->SendPacket(sessionId, packet);
	}}

    packet->Free();
}}
";


        // ============================================================
        // Stub Header
        // ============================================================

        public static string stubHeader =
        @"#pragma once
#include <atomic>
#include ""../Utils/Packet.h""

class RpcStub
{{
public:

    bool PacketProc(uint64_t sessionId, Packet* packet);
{0}
}};
";

        public static string stubHeaderFunc =
@"
    virtual bool {0}(uint64_t sessionId{1});";

        public static string stubHeaderMessageCount =
@"
    std::atomic<int> messageCount_{0}_;";

        // ============================================================
        // Stub cpp
        // ============================================================

        public static string stubCppHeader =
@"#include ""RpcStub.h""
";

        public static string stubCppPacketProc =
@"
bool RpcStub::PacketProc(uint64_t sessionId, Packet* packet)
{{
    unsigned short type;
    *packet >> type;

    switch (type)
    {{{0}
    default:
    {{
        return false;
    }}
    }}
}}
";

        public static string stubPacketProcCase =
        @"
    case {0}:
    {{
        messageCount_{3}_++;

{1}
        *packet{2};

        if (packet->IsError())
            return false;

        return {4}(sessionId{5});
    }}";

        public static string shiftRight =
@" >> {0}";

        // 각 RPC 함수의 기본 구현
        public static string stubCppFunc =
@"
bool RpcStub::{0}(uint64_t sessionId{1})
{{
    return true;
}}
";
    }
}
