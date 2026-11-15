#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Mswsock.lib")

#include <WS2tcpip.h>
#include <mstcpip.h>
#include <process.h>
#include "NetServer.h"
#include "../Utils/Packet.h"
#include "../Utils/Logger.h"
#include "../Utils/Profiler.h"
#include <Windows.h>


NetServer::Session::Session()
{
	//acceptOverlapped_.type_ = IO_TYPE::ACCEPT;
	//disconnectOverlapped_.type_ = IO_TYPE::DISCONNECT;
	sendOverlapped_.type_ = IO_TYPE::SEND;
	recvOverlapped_.type_ = IO_TYPE::RECV;

	//recvPacket_ = packetPool_.Alloc();
}

NetServer::Session::~Session()
{
	//packetPool_.Free(recvPacket_);
}


void NetServer::Session::Initialize(uint64_t sessionId, std::string& ip, int port)
{
	//socket_ = socket;
	sessionId_ = sessionId;

	ip_ = ip;
	port_ = port;

	// 초기화시 참조 카운트를 1로 시작
	ioCount_ = 1;
	sendPending_ = false;
}

void NetServer::Session::Clear()
{
	//sendQueue_.Clear();
	while (1)
	{
		Packet* packet;

		bool ret = sendQueue_.Dequeue(packet);

		if (ret == false)
			break;

		//delete p;
		packet->Free();
	}

	//sendPostQueue_.Clear();
	while (1)
	{
		Packet* packet;

		bool ret = sendPostQueue_.Dequeue(packet);

		if (ret == false)
			break;

		//delete p;
		packet->Free();
	}

	recvQueue_.Clear();
	

	//recvPacket_->Clear(sizeof(LAN_HEADER));
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


	hNetworkIOCP_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, concurrentCount_);

	/// 리슨소켓을 IOCP에 등록
	CreateIoCompletionPort((HANDLE)listenSocket_, hNetworkIOCP_, (u_long)0, 0);

	for (int i = 0; i < workerCount_; i++)
	{
		HANDLE hThread = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, this, 0, nullptr);
		hWorkerThread_.push_back(hThread);
	}

	/// send 스레드
	hSendIOCP_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, SEND_CONCURRENT_COUNT);

	for (int i = 0; i < SEND_WORKER_COUNT + 0; i++)
	{
		HANDLE hThread = (HANDLE)_beginthreadex(nullptr, 0, SendThread, this, 0, nullptr);
		hSendThread_.push_back(hThread);
	}


	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	InetPtonA(AF_INET, serverIp_.c_str(), &serverAddr.sin_addr);
	serverAddr.sin_port = htons(serverPort_);

	if (bind(listenSocket_, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		wprintf(L"bind() error");

	/// 백로그큐 만개
	if (listen(listenSocket_, SOMAXCONN_HINT(10000)) == SOCKET_ERROR)
		wprintf(L"listen() error");


	//LPFN_ACCEPTEX lpfnAcceptEx_ = NULL;
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes;

	int ret = WSAIoctl(listenSocket_, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx, sizeof(GuidAcceptEx),
		&lpfnAcceptEx_, sizeof(lpfnAcceptEx_),
		&dwBytes, NULL, NULL);

	if (ret == SOCKET_ERROR)
	{
		wprintf(L"WSAIoctl failed with error: %u\n", WSAGetLastError());
		__debugbreak();
	}


	//LPFN_ACCEPTEX lpfnAcceptEx_ = NULL;
	GUID GuidDisconnectEx = WSAID_DISCONNECTEX;
	DWORD dwBytes1;

	int ret1 = WSAIoctl(listenSocket_, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidDisconnectEx, sizeof(GuidDisconnectEx),
		&lpfnDisconnectex_, sizeof(lpfnDisconnectex_),
		&dwBytes1, NULL, NULL);

	if (ret1 == SOCKET_ERROR)
	{
		wprintf(L"WSAIoctl failed with error: %u\n", WSAGetLastError());
		__debugbreak();
	}


	//sessions_ = new Session * [sessionMax_];

	//for (int i = 0; i < sessionMax_; i++)
	//{
	//	/// 세션 생성 및 인덱스 설정
	//	sessions_[i] = new Session();
	//	sessions_[i]->sessionId_ = static_cast<uint64_t>(i) << 48;

	//	/// 소켓 생성
	//	SOCKET acceptSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//	sessions_[i]->socket_ = acceptSocket;
	//}

	//for (int i = 0; i < sessionMax_; i++)
	//{
	//	Session* session = sessions_[i];

	//	/// AccceptEx 호출
	//	AcceptPost(session);
	//}


	/// Accept Thread
	for (int i = 0; i < sessionMax_; i++)
	{
		sessionIndexPool_.Push(sessionMax_ - i - 1);
	}

	for (int i = 0; i < sessionMax_; i++)
	{
		sessions_[i] = new Session();
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
		PostQueuedCompletionStatus(hNetworkIOCP_, 0, 0, nullptr);
	}

	WaitForMultipleObjects(workerCount_, hWorkerThread_.data(), TRUE, INFINITE);


	/// send thread 종료
	for (int i = 0; i < SEND_WORKER_COUNT; i++)
	{
		PostQueuedCompletionStatus(hSendIOCP_, 0, 0, nullptr);
	}


	for (int i = 0; i < workerCount_; i++)
	{
		CloseHandle(hWorkerThread_[i]);
	}

	// 세션맵 정리
	//for (auto& session : sessionMap_)
	//{
	//	delete session.second;
	//}

	for (int i = 0; i < sessionMax_; i++)
	{
		delete sessions_[i];
	}

	//sessionIndex_.Clear();

	// IOCP 삭제
	CloseHandle(hNetworkIOCP_);

	LOG_INFO(L"[NETWORK] server exit");
}


bool NetServer::Disconnect(uint64_t sessionId)
{
	Session* session = sessions_[GetSessionIndex(sessionId)];

	// 세션 보장용 카운트 증가
	while (1)
	{
		int count = session->ioCount_;

		if (count & RELEASE_MASK)
		{
			return false;
		}

		if (InterlockedCompareExchange(&session->ioCount_, count + 1, count) == count)
		{
			break;
		}
	}

	// 새로운 세션으로 잘못 얻었다면
	if (session->sessionId_ != sessionId)
	{
		// 새로운 세션의 카운트 감소
		ReleaseSession(session);

		return false;
	}


	shutdown(session->socket_, SD_BOTH);


	// 세션 보장용 카운트 감소
	ReleaseSession(session);

	disconnectCountTotal_++;

	return true;
}

bool NetServer::SendPacket(uint64_t sessionId, Packet* packet)
{
	if (packet == nullptr)
		__debugbreak();

	Session* session = sessions_[GetSessionIndex(sessionId)];

	// 세션 보장용 카운트 증가
	while (1)
	{
		// 스택으로 사용
		int count = session->ioCount_;

		if (count & RELEASE_MASK)
		{
			return false;
		}

		if (InterlockedCompareExchange(&session->ioCount_, count + 1, count) == count)
		{
			break;
		}
	}

	// 새로운 세션으로 잘못 얻었다면
	if (session->sessionId_ != sessionId)
	{
		// 새로운 세션의 카운트 감소
		ReleaseSession(session);

		return false;
	}


	///헤더 변경시 수정//////////////////////////////////////////////////////////////////////////////////////
	if (!packet->IsEncoding())
	{
		packet->SetNetHeader();

		packet->Encoding();
	}


	/// char* 링버퍼 용도
	//session->sendQueue_.Enqueue(packet->GetBufferPtr(), packet->TotalUseSize());
	//delete packet;

	/// Packet* 링버퍼 용도
	packet->AddRef();
	session->sendQueue_.Enqueue(packet);

	if (session->sendQueue_.Size() > 2000)
	{
		Disconnect(sessionId);

		ReleaseSession(session);

		return false;
	}

	sendMessageCount_++;

	/// 기존 방식
	SendPost(session);

	/// TODO: Send Thread 전달
	//InterlockedIncrement(&session->ioCount_);
	//PostQueuedCompletionStatus(hSendIOCP_, SEND_MESSAGE_NUM, (ULONG_PTR)session, (LPOVERLAPPED)SEND_MESSAGE_NUM);


	// 세션 보장용 카운트 감소
	ReleaseSession(session);

	return true;
}


unsigned int __stdcall NetServer::AcceptThread(void* param)
{
	NetServer* server = (NetServer*)param;

	server->AcceptFunc();

	return 0;
}

void NetServer::AcceptFunc()
{
	while (1)
	{
		SOCKET clientSock = accept(listenSocket_, nullptr, nullptr);
		if (clientSock == INVALID_SOCKET)
		{
			int error = WSAGetLastError();

			// 10038 WSAENOTSOCK
			if (error == 10038)
				break;

			wprintf(L"accept() error: %d", error);

			continue;
		}

		CompleteAccept(clientSock);
	}
}

void NetServer::CompleteAccept(SOCKET socket)
{
	acceptCount_++;
	acceptCountTotal_++;

	SOCKADDR_IN clientAddr;
	int len = sizeof(clientAddr);

	getpeername(socket, (SOCKADDR*)&clientAddr, &len);

	char str[16];
	InetNtopA(AF_INET, &clientAddr.sin_addr, str, 16);
	std::string ip = str;
	int port = ntohs(clientAddr.sin_port);

	// 허용되지 않은 ip와 port라면 바로 연결을 끊음
	if (!OnConnectionRequest(ip, port))
	{
		closesocket(socket);

		return;
	}

	//sessionMapLock_.lock();
	//index = sessionIndex_.top();
	//sessionIndex_.pop();
	//sessionMapLock_.unlock();

	int index;
	sessionIndexPool_.Pop(index);

	uint64_t sessionId = ++idSeed_ | (static_cast<uint64_t>(index) << 48);

	Session* session = sessions_[index];

	session->Initialize(sessionId, ip, port);
	session->socket_ = socket;	

	InterlockedIncrement(&sessionCount_);

	//LOG_INFO(L"[NETWORK] session create id=0x%016llx ip=%S port=%d count=%d", sessionId, ip.c_str(), port, sessionCount_);

	CreateIoCompletionPort((HANDLE)socket, hNetworkIOCP_, (ULONG_PTR)session, 0);


	OnAccept(session->sessionId_);

	RecvPost(session);
}


//void NetServer::AcceptPost(Session* session)
//{
//	ZeroMemory(&session->acceptOverlapped_.overlapped_, sizeof(WSAOVERLAPPED));
//
//	/// AcceptEx 등록
//	BOOL ret = lpfnAcceptEx_(listenSocket_, session->socket_, session->acceptBuffer,
//		0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
//		nullptr, (WSAOVERLAPPED*)(&session->acceptOverlapped_));
//
//	if (ret == FALSE)
//	{
//		int error = WSAGetLastError();
//
//		if (error == ERROR_IO_PENDING)
//		{
//			return;
//		}
//		else
//		{
//			wprintf(L"AcceptEx failed with error: %d\n", error);
//			__debugbreak();
//		}
//	}
//}
//
//void NetServer::CompleteAccept(Session* session)
//{
//	acceptCount_++;
//	acceptCountTotal_++;
//
//	SOCKET acceptSocket = session->socket_;
//
//	/// 리슨소켓의 속성 상속
//	setsockopt(acceptSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listenSocket_, sizeof(listenSocket_));
//
//	SOCKADDR_IN clientAddr;
//	int len = sizeof(clientAddr);
//
//	getpeername(acceptSocket, (SOCKADDR*)&clientAddr, &len);
//
//	char str[16];
//	InetNtopA(AF_INET, &clientAddr.sin_addr, str, 16);
//	std::string ip = str;
//	int port = ntohs(clientAddr.sin_port);
//
//	// 허용되지 않은 ip와 port라면 바로 연결을 끊음
//	if (!OnConnectionRequest(ip, port))
//	{
//		closesocket(acceptSocket);
//
//		return;
//	}
//
//	uint64_t sessionId = AllocSessionId(session);
//
//	//Session* session = sessions[GetSessionIndex(sessionId)];
//
//	session->Initialize(sessionId, ip, port);
//
//	InterlockedIncrement(&(sessionCount_));
//
//	//LOG_INFO(L"[NETWORK] session create ip=%S port=%d count=%d", ip.c_str(), port, server->sessionCount_);
//
//	/// AcceptEx 완료 후에 IOCP에 등록
//	CreateIoCompletionPort((HANDLE)acceptSocket, hIOCP_, (ULONG_PTR)session, 0);
//
//
//	OnAccept(session->sessionId_);
//
//	RecvPost(session);
//}
//
//
//void NetServer::CompleteDisconnect(Session* session)
//{
//	session->Clear();
//
//	AcceptPost(session);
//}


unsigned int __stdcall NetServer::WorkerThread(void* param)
{
	NetServer* server = (NetServer*)param;

	server->WorkerFunc();

	return 0;
}

void NetServer::WorkerFunc()
{
	DWORD numOfBytes;
	Session* session;
	OVERLAPPED* overlapped_;

	while (1)
	{
		//session = nullptr;
		//numOfBytes = 0;

		// GQCS 호출 반환시 overlapped 구조체 무조건 세팅
		BOOL ret = GetQueuedCompletionStatus(hNetworkIOCP_, &numOfBytes, (PULONG_PTR)&session, &overlapped_, INFINITE);

		//PRO(L"GQCS()");

		if (overlapped_ == nullptr && session == nullptr && numOfBytes == 0)
		{
			LOG_INFO(L"[NETWORK] worker thread exit");

			return;
		}

		if (!ret)
		{
			int error = GetLastError();

			// 실패한 IO (연결 끊김)
			// 완료 패킷을 꺼내서, 실패한 IO에 대한 정보를 매개변수에 저장함
			if (overlapped_ != nullptr)
			{
				// 64 상대가 연결을 끊었을때
				if (error == ERROR_NETNAME_DELETED)
				{
					ReleaseSession(session);

					continue;
				}
				// 1236 내가 연결을 끊었을때 (closesocket)
				// 세션 정리때 소켓을 닫는 구조라 이 에러가 정상적인 상황에선 불가능
				else if (error == ERROR_CONNECTION_ABORTED)
				{
					__debugbreak();

					ReleaseSession(session);

					continue;
				}
				/// 어떻게 처리해야할까??
				// 121 트래픽 많으면 네트워크 연결 끊어버림
				// 리모트환경 테스트시 발생
				// 트래픽 줄여서 다시 테스트
				else if (error == ERROR_SEM_TIMEOUT)
				{
					__debugbreak();

					ReleaseSession(session);

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
					return;
				}
				else
				{
					// GQCS 잘못 호출
					// 이 상황 배제
					return;
				}
			}
		}

		IO_TYPE type = ((OverlappedEx*)overlapped_)->type_;

		if (type == IO_TYPE::RECV)
		{
			// 상대가 closesocket()시 (rst x) 발생
			if (numOfBytes == 0)
			{
				__debugbreak();

				ReleaseSession(session);
			}
			else
			{
				CompleteRecv(session, numOfBytes);
			}
		}

		else if (type == IO_TYPE::SEND)
		{
			CompleteSend(session, numOfBytes);
		}

		//else if (type == IO_TYPE::ACCEPT)
		//{
		//	/// Accept 완료 통지는 Completion Key로 listenSocket이 오므로 반드시 따로 처리
		//	Session* sess = reinterpret_cast<Session*>(overlapped_);

		//	CompleteAccept(sess);
		//}

		//else if (type == IO_TYPE::DISCONNECT)
		//{
		//	CompleteDisconnect(session);
		//}

		else
		{
			__debugbreak();
		}
	}
}


unsigned int __stdcall NetServer::SendThread(void* param)
{
	NetServer* server = (NetServer*)param;

	server->SendFunc();

	return 0;
}

void NetServer::SendFunc()
{
	DWORD numOfBytes;
	Session* session;
	OVERLAPPED* overlapped_;

	while (1)
	{
		//session = nullptr;
		//numOfBytes = 0;

		// GQCS 호출 반환시 overlapped 구조체 무조건 세팅
		BOOL ret = GetQueuedCompletionStatus(hSendIOCP_, &numOfBytes, (PULONG_PTR)&session, &overlapped_, INFINITE);

		//PRO(L"GQCS()");

		if (overlapped_ == nullptr && session == nullptr && numOfBytes == 0)
		{
			LOG_INFO(L"[NETWORK] worker thread exit");

			return;
		}

		if (overlapped_ == (LPOVERLAPPED)SEND_MESSAGE_NUM && numOfBytes == SEND_MESSAGE_NUM)
		{
			SendPost(session);

			ReleaseSession(session);

			continue;
		}

		if (!ret)
		{
			int error = GetLastError();

			// 실패한 IO (연결 끊김)
			// 완료 패킷을 꺼내서, 실패한 IO에 대한 정보를 매개변수에 저장함
			if (overlapped_ != nullptr)
			{
				// 64 상대가 연결을 끊었을때
				if (error == ERROR_NETNAME_DELETED)
				{
					ReleaseSession(session);

					continue;
				}
				// 1236 내가 연결을 끊었을때 (closesocket)
				// 세션 정리때 소켓을 닫는 구조라 이 에러가 정상적인 상황에선 불가능
				else if (error == ERROR_CONNECTION_ABORTED)
				{
					__debugbreak();

					ReleaseSession(session);

					continue;
				}
				/// 어떻게 처리??
				// 121 트래픽 많으면 네트워크 연결 끊어버림
				// 리모트환경 테스트시 발생
				// 트래픽 줄여서 다시 테스트
				else if (error == ERROR_SEM_TIMEOUT)
				{
					ReleaseSession(session);

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
					return;
				}
				else
				{
					// GQCS 잘못 호출
					// 이 상황 배제
					return;
				}
			}
		}
	}
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

