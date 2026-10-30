#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

#include <WS2tcpip.h>
#include <mstcpip.h>
#include <process.h>
#include "NetServer.h"
#include "../Utils/Packet.h"
#include "../Utils/Logger.h"
#include "../Utils/Profiler.h"
#include <Windows.h>

void NetServer::Session::Initialize(SOCKET socket, std::string ip, int port, uint64_t id)
{
	socket_ = socket;
	sessionId_ = id;

	ip_ = ip;
	port_ = port;

	sendQueue_.Clear();
	sendPostQueue_.Clear();

	//recvQueue_.Clear();
	recvPacket_ = packetPool_.Alloc();
	recvPacket_->InitRecvPacket(sizeof(LAN_HEADER));

	sendOverlapped_.type = IO_TYPE::SEND;
	recvOverlapped_.type = IO_TYPE::RECV;

	// 초기화시 참조 카운트를 1로 시작
	ioCount_ = 1;
	sendPending_ = false;
}

NetServer::NetServer()
{
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		wprintf(L"WSAStartup() error");
}

NetServer::~NetServer()
{
	WSACleanup();
}

bool NetServer::Start(std::string ip, int port, int sessionMax, int concurrentCount, int workerCount)
{
	serverIp_ = ip;
	serverPort_ = port;
	sessionMax_ = sessionMax;
	//sessionMap_.reserve(sessionMax_);

	for (int i = 0; i < 200; i++)
	{
		sessions[i] = new Session();
	}

	for (int i = 0; i < 200; i++)
	{
		//sessionIndex_.push(200 - i - 1);
		sessionIndex_.Push(sessionMax_ - i - 1);
	}

	concurrentCount_ = concurrentCount;
	workerCount_ = workerCount;

	listenSocket_ = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket_ == INVALID_SOCKET)
		wprintf(L"socket() error");

	/// 링거 - closesocekt() 호출시 즉시 리턴, 연결 강제 종료
	// 서버 클라 둘다 설정할 것
	LINGER lin;
	lin.l_onoff = 1;
	lin.l_linger = 0;
	setsockopt(listenSocket_, SOL_SOCKET, SO_LINGER, (char*)&lin, sizeof(lin));

	/// 네이글 off
	//int nagleFlag = 1;
	//setsockopt(listenSocket_, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&nagleFlag), sizeof(nagleFlag));

	/// TCP Keep-Alive 활성화
	//BOOL keepAlive = TRUE;
	//setsockopt(listenSocket_, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&keepAlive), sizeof(keepAlive));

	//// Keep-Alive 설정
	//tcp_keepalive keepAliveVals{};
	//keepAliveVals.onoff = 1;
	//keepAliveVals.keepalivetime = 5000;   // 5초
	//keepAliveVals.keepaliveinterval = 1000; // 응답 없을 때 1초 간격

	//DWORD bytesReturned = 0;

	//WSAIoctl(listenSocket_, SIO_KEEPALIVE_VALS, &keepAliveVals, sizeof(keepAliveVals), nullptr, 0, &bytesReturned, nullptr, nullptr);


	/// L4 송신버퍼 사이즈
	//int sndBufSize = 0;
	//setsockopt(listenSocket_, SOL_SOCKET, SO_SNDBUF, (const char*)(&sndBufSize), sizeof(sndBufSize));


	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	InetPtonA(AF_INET, serverIp_.c_str(), &serverAddr.sin_addr);
	serverAddr.sin_port = htons(serverPort_);

	if (bind(listenSocket_, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		wprintf(L"bind() error");

	if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR)
		wprintf(L"listen() error");

	hIOCP_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, concurrentCount_);

	for (int i = 0; i < workerCount_; i++)
	{
		HANDLE hThread = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, this, 0, nullptr);
		hWorkerThread_.push_back(hThread);
	}

	hAcceptThread_ = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, this, 0, nullptr);

	LOG_INFO(L"[NETWORK] server start");

	return true;
}

void NetServer::Stop()
{
	// accept thread 종료
	closesocket(listenSocket_);
	WaitForSingleObject(hAcceptThread_, INFINITE);
	CloseHandle(hAcceptThread_);

	// worker thread 종료
	for (int i = 0; i < workerCount_; i++)
	{
		PostQueuedCompletionStatus(hIOCP_, 0, 0, nullptr);
	}

	WaitForMultipleObjects(workerCount_, hWorkerThread_.data(), TRUE, INFINITE);

	for (int i = 0; i < workerCount_; i++)
	{
		CloseHandle(hWorkerThread_[i]);
	}

	// 세션맵 정리
	//for (auto& session : sessionMap_)
	//{
	//	delete session.second;
	//}

	for (int i = 0; i < 20000; i++)
	{
		delete sessions[i];
	}

	sessionIndex_.Clear();

	// IOCP 삭제
	CloseHandle(hIOCP_);

	LOG_INFO(L"[NETWORK] server exit");
}

int NetServer::SessionCount() const
{
	return sessionCount_;
}

int NetServer::SessionMax() const
{
	return sessionMax_;
}

bool NetServer::Disconnect(uint64_t sessionId)
{
	Session* session = sessions[GetSessionIndex(sessionId)];

	closesocket(session->socket_);

	return true;
}

// lock_guard 사용시, sessionLock이 삭제될 위험 있으므로 사용 안함
bool NetServer::SendPacket(uint64_t sessionId, Packet* packet)
{
	Session* session = sessions[GetSessionIndex(sessionId)];

	// 세션 보장용 카운트 증가
	InterlockedIncrement(&session->ioCount_);

	if (session->sessionId_ != sessionId)
	{
		ReleaseSession(session);

		return false;
	}

	if (session->ioCount_ < 0)
	{
		ReleaseSession(session);

		return false;
	}

	///헤더 변경시 수정//////////////////////////////////////////////////////////////////////////////////////
	packet->SetLanHeader();

	/// char* 링버퍼 용도
	//session->sendQueue_.Enqueue(packet->GetBufferPtr(), packet->TotalUseSize());
	//delete packet;

	/// Packet* 링버퍼 용도
	session->sendQueue_.Enqueue(packet);

	// TODO: 값 수정
	if (session->sendQueue_.Size() > 2000)
	{
		Disconnect(sessionId);

		return false;
	}

	InterlockedIncrement(&sendMessageCount_);

	this->SendPost(session);

	// 세션 보장용 카운트 감소
	ReleaseSession(session);

	return true;
}


unsigned int __stdcall NetServer::AcceptThread(void* param)
{
	NetServer* server = (NetServer*)param;

	while (1)
	{
		SOCKADDR_IN clientAddr;
		int addrLen;
		addrLen = sizeof(clientAddr);

		SOCKET clientSock = accept(server->listenSocket_, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSock == INVALID_SOCKET)
		{
			int error = WSAGetLastError();

			if (error == 10038)
				break;

			wprintf(L"accept() error: %d", error);

			continue;
		}

		InterlockedIncrement(&server->acceptCount_);

		CHAR str[16];
		InetNtopA(AF_INET, &clientAddr.sin_addr, str, 16);
		std::string ip = str;
		int port = ntohs(clientAddr.sin_port);

		// 허용되지 않은 ip와 port라면 바로 연결을 끊음
		if (!server->OnConnectionRequest(ip, port))
		{
			closesocket(clientSock);
			continue;
		}

		uint64_t sessionId = server->AllocSession();

		Session* session = server->sessions[server->GetSessionIndex(sessionId)];

		session->Initialize(clientSock, ip, port, sessionId);

		InterlockedIncrement(&(server->sessionCount_));

		//LOG_INFO(L"[NETWORK] session create ip=%S port=%d count=%d", ip.c_str(), port, server->sessionCount_);

		CreateIoCompletionPort((HANDLE)clientSock, server->hIOCP_, (ULONG_PTR)session, 0);

		server->OnAccept(session->sessionId_);

		server->RecvPost(session);
	}

	return 0;
}

unsigned int __stdcall NetServer::WorkerThread(void* param)
{
	NetServer* server = (NetServer*)param;

	DWORD numOfBytes;
	Session* session;
	OVERLAPPED* overlapped;

	while (1)
	{
		//session = nullptr;
		//numOfBytes = 0;

		// GQCS 호출 반환시 overlapped 구조체 무조건 세팅
		BOOL ret = GetQueuedCompletionStatus(server->hIOCP_, &numOfBytes, (PULONG_PTR)&session, &overlapped, INFINITE);

		//PRO(L"GQCS()");

		if (overlapped == nullptr && session == nullptr && numOfBytes == 0)
		{
			LOG_INFO(L"[NETWORK] worker thread exit");

			return 0;
		}

		if (!ret)
		{
			int error = GetLastError();

			// 실패한 IO (연결 끊김)
			// 완료 패킷을 꺼내서, 실패한 IO에 대한 정보를 매개변수에 저장함
			if (overlapped != nullptr)
			{
				// 64 상대가 연결을 끊었을때 (numOfBytes == 0)
				// 1236 clientSock 닫았을때
				if (error == ERROR_NETNAME_DELETED || error == ERROR_CONNECTION_ABORTED)
				{
					server->ReleaseSession(session);

					continue;
				}
				// 121 트래픽 많으면 네트워크 연결 끊어버림
				// 리모트환경 테스트시 발생
				// 트래픽 줄여서 다시 테스트
				else if (error == ERROR_SEM_TIMEOUT)
				{
					//__debugbreak();

					continue;
				}

				LOG_INFO(L"GQCS() error: %d", error);

				__debugbreak();

				continue;
			}
			// 완료 패킷을 큐에서 제거 안함, 매개변수 값 의미 x
			else
			{
				if (error == WAIT_TIMEOUT)
				{
					continue;
				}
				// 735 CP 삭제시
				else if (error == ERROR_ABANDONED_WAIT_0)
				{
					// 스레드 종료
					return 0;
				}
				else
				{
					// GQCS 잘못 호출
					// 이 상황 배제
					return 0;
				}
			}
		}

		if (((OverlappedEx*)overlapped)->type == IO_TYPE::RECV)
		{
			// 상대가 closesocket()시 (rst x) 발생
			if (numOfBytes == 0)
			{
				server->ReleaseSession(session);
			}
			else
			{
				server->CompleteRecv(session, numOfBytes);
			}
		}
		else
		{
			server->CompleteSend(session, numOfBytes);
		}
	}

	return 0;
}

void NetServer::Monitoring()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	// 콘솔창 커서 숨기기
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);

	cursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo(hConsole, &cursorInfo);

	LONG acceptTps = InterlockedExchange(&acceptCount_, 0);
	LONG recvMessageTps = InterlockedExchange(&recvMessageCount_, 0);
	LONG sendMessageTps = InterlockedExchange(&sendMessageCount_, 0);

	system("cls");

	printf("=====================\n");
	printf(" SERVER  MONITORING\n");
	printf("=====================\n\n");

	printf("Acpt TPS : %d\n", acceptTps);
	printf("Recv TPS : %d\n", recvMessageTps);
	printf("Send TPS : %d\n", sendMessageTps);
}

// ===================================================================
// Socket Error Code
// ===================================================================
// 
// 10014 WSAEFAULT
// 잘못된 버퍼 주소나 버퍼 길이가 너무 작음
// 
// 10022 WSAEINVAL
// wsasend()시 진행 중인 OVERLAPPED 재사용이거나 잘못된 WSABUF
// 
// 10035 WSAEWOULDBLOCK
// L4 버퍼가 다 찼는데 논블로킹 소켓에서 send 호출시
// 논블로킹 소켓에서 send recv 에서 할게 없을때 바로 반환시
// 
// 10038 WSAENOTSOCK
// 할당되지 않은 소켓(or closesocket)에 소켓 api 호출시
// 
// 10053 WSAECONNABORTED
// accept 호출했는데 이미 클라이언트가 종료한 연결일 경우
// 
// 10054 WSAECONNRESET
// rst 보낸 후 send or recv
// fin을 보냈는데 다른쪽에서 send 하면 rst 받음
// 
// 10055 WSAENOBUFS
// 리소스 고갈
// 
// 10057 WSAENOTCONN
// 논블로킹 소켓에서 connect 이후(무조건 반환) 실제로는 연결 안되었을때 send시
// 
// 10060 WSAETIMEDOUT
// 연결 중 인터넷 끊어지면 recv할때 에러 발생(send는 단순 복사니깐)
// 
// 리모트 환경에서 안 열린 포트로 connect ?
// 서버 백로그큐 다 차면 서버 컴퓨터는 아무것도 하지 않음 ?
// * 연결 시도에 대한 응답이 일정 시간 동안 오지 않을 떄
// 
// 
// 10061 WSAECONNREFUSED
// 로컬 환경에서 안 열린 포트로 connect
// 서버 백로그큐 다 차면 아무것도 안하고 상대한테 rst 보냄
// * 해당 주소/포트에서 연결을 받아주는 서비스가 없을 때
// 
// 10063 WSAENAMETOOLONG
// 
// 10065 WSAEHOSTUNREACH
// 인터넷 연결 안되었는데 connect
