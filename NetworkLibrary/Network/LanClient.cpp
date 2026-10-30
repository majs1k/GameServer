#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

#include "LanClient.h"
#include "../Utils/Packet.h"
#include "../Utils/Logger.h"
#include "../Utils/Profiler.h"

LanClient::LanClient()
{
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		wprintf(L"WSAStartup() error");
}

LanClient::~LanClient()
{
	delete session_;

	WSACleanup();
}

bool LanClient::Connect(std::wstring ip, int port)
{
	SOCKET clientSocket_ = socket(PF_INET, SOCK_STREAM, 0);
	if (clientSocket_ == INVALID_SOCKET)
		wprintf(L"socket() error!");

	// 링거 옵션
	LINGER optval;
	optval.l_onoff = 1;
	optval.l_linger = 0;
	if (setsockopt(clientSocket_, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval)) == SOCKET_ERROR)
		wprintf(L"setsockopt() error!");

	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	InetPton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

	if (connect(clientSocket_, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		wprintf(L"connect() error!");

	// connect 이후 논블로킹 옵션
	u_long on = 1;
	if (ioctlsocket(clientSocket_, FIONBIO, &on) == SOCKET_ERROR)
		wprintf(L"iocltsocket error!");


	session_ = new Session();
	// 세션 초기화
	session_->socket_ = clientSocket_;
	session_->ip_ = ip;
	session_->port_ = port;

	return true;
}

bool LanClient::Disconnect()
{
	return true;
}

bool LanClient::SendPacket(uint64_t sessionId, Packet* packet)
{
	return true;
}

void LanClient::RecvPost()
{
}

void LanClient::CompleteRecv(int numOfBytes)
{
}

void LanClient::SendPost()
{
}
