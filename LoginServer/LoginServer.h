#pragma once
#include "Network/NetServer.h"
#include "Utils/Packet.h"

#include "CommonProtocol.h"
#include "RPC/RpcStub.h"
#include "Rpc/RpcProxy.h"


class LoginServer : public NetServer, public RpcStub
{
public:

	LoginServer();
	~LoginServer();

private:

	bool OnConnectionRequest(const std::string& ip, int port) override;
	void OnAccept(uint64_t sessionId) override;
	void OnRelease(uint64_t sessionId) override;
	void OnRecv(uint64_t sessionId, Packet* packet) override;

	// ===================================================================================== //
	// Thraed
	// ===================================================================================== //
private:

	static void MonitorThread(LoginServer* server);
	void Monitoring();

	std::thread monitorThread_;

	// ===================================================================================== //
	// Protocol
	// ===================================================================================== //
private:

	virtual bool OnRecvPacket(uint64_t sessionId, Packet* packet, ushort protocol)
	{
		return true;
	}

	// ===================================================================================== //
	// RPC
	// ===================================================================================== //
private:

	bool CS_ReqLogin(uint64_t sessionId, int64_t userId, SessionKey& sessionKey) override;

	RpcProxy proxy_;

	// ===================================================================================== //
	// DB
	// ===================================================================================== //
private:

	void CreateTable();
};
