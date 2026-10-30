#pragma once
#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "../Utils/RingBuffer.h"

#define SEND_SIZE				10000
#define RECV_SIZE				10000


class Packet;

class LanClient
{

private:

	struct Session
	{
		SOCKET socket_;

		std::wstring ip_;
		int port_;

		RingBuffer sendQueue_{ SEND_SIZE };
		RingBuffer recvQueue_{ RECV_SIZE };
	};

private:

	Session* session_;

	std::wstring serverIp_;
	int serverPort_;

public:

	LanClient();
	virtual ~LanClient();

	bool Connect(std::wstring ip, int port);
	bool Disconnect();
	bool SendPacket(uint64_t sessionId, Packet* packet);

	virtual void OnConnect() = 0;
	virtual void OnRelease() = 0;
	virtual void OnRecv(Packet* packet) = 0;
	//virtual void OnError(int errorCode, wchar_t* str) = 0;

private:

	void RecvPost();
	void CompleteRecv(int numOfBytes);
	void SendPost();
};
