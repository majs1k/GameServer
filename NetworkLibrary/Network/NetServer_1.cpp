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
	ioCount_ = 1;
	sendPending_ = false;

	recvQueue_.Clear();

	//Packet* packet = packetPool_.Alloc();
	//packet->Clear(sizeof(NET_HEADER));
	//recvPacket_ = packet;
}

void NetServer::Session::Clear()
{
	//sendQueue_.clear();
	while (1)
	{
		Packet* packet;

		bool ret = sendQueue_.Dequeue(packet);

		if (ret == false)
			break;

		//delete p;
		packet->Free();
	}

	while (1)
	{
		if (sendPostQueue_.empty())
			break;

		Packet* packet = sendPostQueue_.front();
		sendPostQueue_.pop();

		//delete p;
		packet->Free();
	}
}

NetServer::NetServer()
{
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		__debugbreak();
	}
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
	{
		__debugbreak();
	}

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
	// send 완료통지가 성공해야(ACK가 와야) 해당 소켓으로 다시 send 가능
	// 페이지락 비용이 커널 복사 비용보다 클 수 있음?
	//int sndBufSize = 0;
	//setsockopt(listenSocket_, SOL_SOCKET, SO_SNDBUF, (const char*)(&sndBufSize), sizeof(sndBufSize));


	recvCompletionPort_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, concurrentCount_);

	/// 리슨소켓을 IOCP에 등록
	CreateIoCompletionPort((HANDLE)listenSocket_, recvCompletionPort_, (u_long)0, 0);

	for (int i = 0; i < workerCount_; i++)
	{
		HANDLE hThread = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, this, 0, nullptr);
		recvThread_.push_back(hThread);
	}

	/// send 스레드
	sendCompletionPort_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, SEND_CONCURRENT_COUNT);

	for (int i = 0; i < SEND_WORKER_COUNT + 0; i++)
	{
		HANDLE hThread = (HANDLE)_beginthreadex(nullptr, 0, SendThread, this, 0, nullptr);
		sendThreads_.push_back(hThread);
	}

	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	InetPtonA(AF_INET, serverIp_.c_str(), &serverAddr.sin_addr);
	serverAddr.sin_port = htons(serverPort_);

	if (bind(listenSocket_, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		printf("bind() error: %d", WSAGetLastError());
		__debugbreak();
	}

	/// 백로그큐
	if (listen(listenSocket_, SOMAXCONN_HINT(10000)) == SOCKET_ERROR)
	{
		printf("listen() error: %d", WSAGetLastError());
		__debugbreak();
	}


	//LPFN_ACCEPTEX lpfnAcceptEx_ = NULL;
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes;

	int ret = WSAIoctl(listenSocket_, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx),
		&lpfnAcceptEx_, sizeof(lpfnAcceptEx_), &dwBytes, NULL, NULL);

	if (ret == SOCKET_ERROR)
	{
		printf("WSAIoctl failed with error: %u\n", WSAGetLastError());
		__debugbreak();
	}

	//GUID GuidDisconnectEx = WSAID_DISCONNECTEX;
	//DWORD dwBytes1;

	//int ret1 = WSAIoctl(listenSocket_, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidDisconnectEx, sizeof(GuidDisconnectEx),
	//	&lpfnDisconnectex_, sizeof(lpfnDisconnectex_), &dwBytes1, NULL, NULL);

	//if (ret1 == SOCKET_ERROR)
	//{
	//	printf("WSAIoctl failed with error: %u\n", WSAGetLastError());
	//	__debugbreak();
	//}

	/// AcceptEx
	//for (int i = 0; i < sessionMax_; i++)
	//{
	//	/// 세션 생성 및 인덱스 설정
	//	Session* session = new Session();
	//	session->sessionId_ = static_cast<uint64_t>(i) << 48;

	//	/// 소켓 생성
	//	SOCKET acceptSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//	session->socket_ = acceptSocket;

	//	/// AccceptEx 호출
	//	AcceptPost(session);

	//	sessions_.push_back(session);
	//}

	/// Accept Thread
	for (int i = 0; i < sessionMax_; i++)
	{
		sessionIndexPool_.Push(sessionMax_ - i - 1);
	}

	for (int i = 0; i < sessionMax_; i++)
	{
		sessions_.push_back(new Session());
	}

	hAcceptThread_ = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, this, 0, nullptr);

	LOG("NETWORK", ELOG::SYSTEM_LEVEL, "NetServer Start");
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
		PostQueuedCompletionStatus(recvCompletionPort_, 0, 0, nullptr);
	}

	WaitForMultipleObjects(workerCount_, recvThread_.data(), TRUE, INFINITE);

	/// send thread 종료
	for (int i = 0; i < SEND_WORKER_COUNT; i++)
	{
		PostQueuedCompletionStatus(sendCompletionPort_, 0, 0, nullptr);
	}

	for (int i = 0; i < workerCount_; i++)
	{
		CloseHandle(recvThread_[i]);
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

	while (1)
	{
		int index;
		bool ret = sessionIndexPool_.Pop(index);

		if (ret == false)
			break;
	}

	// IOCP 삭제
	CloseHandle(recvCompletionPort_);

	LOG("NETWORK", ELOG::SYSTEM_LEVEL, "NetServer Stop");
}

bool NetServer::Disconnect(uint64_t sessionId)
{
	Session* session = sessions_[GetSessionIndex(sessionId)];

	// 세션 보장용 카운트 증가
	while (1)
	{
		int count = session->ioCount_;

		if (count & RELEASE_MASK)
			return false;

		if (InterlockedCompareExchange(&session->ioCount_, count + 1, count) == count)
			break;
	}

	// 새로운 세션으로 잘못 얻었다면
	if (session->sessionId_ != sessionId)
	{
		// 새로운 세션의 카운트 감소
		ReleaseSession(session);
		return false;
	}

	disconnectCountTotal_++;
	shutdown(session->socket_, SD_BOTH);
	//LOG("NETWORK", ELOG::SYSTEM_LEVEL, "session disconn / id=0x%016llx / so=0x%016llx", session->sessionId_, session->socket_);
	// 세션 보장용 카운트 감소
	ReleaseSession(session);
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
			return false;

		if (InterlockedCompareExchange(&session->ioCount_, count + 1, count) == count)
			break;
	}

	// 새로운 세션으로 잘못 얻었다면
	if (session->sessionId_ != sessionId)
	{
		// 세션 보장용 카운트 감소
		ReleaseSession(session);
		return false;
	}

	if (!packet->IsEncoding())
	{
		// HACK: 헤더 변경시 수정
		packet->SetNetHeader();
		packet->Encoding();
	}

	if (session->sendQueue_.Size() >= SEND_SIZE)
	{
		Disconnect(sessionId);
		// 세션 보장용 카운트 감소
		ReleaseSession(session);
		return false;
	}

	/// char* 링버퍼 용도
	//session->sendQueue_.Enqueue(packet->GetBufferPtr(), packet->TotalUseSize());
	//delete packet;

	/// Packet* 링버퍼 용도
	packet->AddRef();
	session->sendQueue_.Enqueue(packet);

	sendMessageCount_++;

	/// 기존 방식
	//SendPost(session);

	/// TODO: Send Thread 전달
	sendWorkCount_++;
	InterlockedIncrement(&session->ioCount_);
	PostQueuedCompletionStatus(sendCompletionPort_, 0, (ULONG_PTR)session, nullptr);

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

			__debugbreak();
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

	// 해외/공격 IP 차단 + 패치 후 점검 white ip
	if (!OnConnectionRequest(ip, port))
	{
		closesocket(socket);
		return;
	}

	if (sessionCount_ >= sessionMax_)
	{
		LOG("NETWORK", ELOG::SYSTEM_LEVEL, "sessions full");
		closesocket(socket);
		return;
	}

	//sessionMapLock_.lock();
	//index = sessionIndex_.top();
	//sessionIndex_.pop();
	//sessionMapLock_.unlock();

	int index;
	sessionIndexPool_.Pop(index);
	Session* session = sessions_[index];

	// 아이디에 세션 배열 인덱스 포함
	uint64_t sessionId = ++idSeed_ | (static_cast<uint64_t>(index) << 48);
	session->Initialize(sessionId, ip, port);
	session->socket_ = socket;

	//LOG("NETWORK", ELOG::SYSTEM_LEVEL, "session create / id=0x%016llx / ip=%s / port=%d", session->sessionId_, ip.c_str(), port);
	//LOG("NETWORK", ELOG::SYSTEM_LEVEL, "session create / id=0x%016llx / so=0x%016llx", session->sessionId_, session->socket_);

	CreateIoCompletionPort((HANDLE)socket, recvCompletionPort_, (ULONG_PTR)session, 0);
	InterlockedIncrement(&sessionCount_);
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
//			printf("AcceptEx failed with error: %d\n", error);
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
//	if (!OnConnectionRequest(ip, port))
//		return;
//
//	uint64_t index = session->sessionId_ >> SESSION_INDEX_SHIFT;
//	uint64_t sessionId = ++idSeed_ | (index << SESSION_INDEX_SHIFT);
//
//	session->Initialize(sessionId, ip, port);
//
//	LOG("NETWORK", ELOG::SYSTEM_LEVEL, "session create / id=0x%016llx / so=0x%016llx", session->sessionId_, session->socket_);
//
//	CreateIoCompletionPort((HANDLE)acceptSocket, hNetworkIOCP_, (ULONG_PTR)session, 0);
//	InterlockedIncrement(&(sessionCount_));
//	OnAccept(session->sessionId_);
//	RecvPost(session);
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
		BOOL ret = GetQueuedCompletionStatus(recvCompletionPort_, &numOfBytes, reinterpret_cast<PULONG_PTR>(&session), &overlapped_, INFINITE);

		if (overlapped_ == nullptr && session == nullptr && numOfBytes == 0)
		{
			LOG("NETWORK", ELOG::SYSTEM_LEVEL, "IO Worker Thread Stop");
			return;
		}

		/// TODO: 테스트
		//if (overlapped_ == (LPOVERLAPPED)PROTOCOL_SEND_PACKET && numOfBytes == PROTOCOL_SEND_PACKET)
		//{
		//	SendPost(session);
		//	ReleaseSession(session);
		//	continue;
		//}

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

				// 1236 내가 연결을 끊었을때
				// 세션 정리때 closesocket을 하므로 지금 구조에선 shutdown 호출시만 발생 가능
				if (error == ERROR_CONNECTION_ABORTED)
				{
					ReleaseSession(session);
					continue;
				}

				// 121 tcp 재전송 최대 횟수 초과시
				if (error == ERROR_SEM_TIMEOUT)
				{
					ReleaseSession(session);
					continue;
				}

				LOG("NETWORK", ELOG::ERROR_LEVEL, "recv thread GQCS() error=%d / id=0x%016llx", error, session->sessionId_);
				__debugbreak();
			}
			// 완료 패킷을 큐에서 제거 안함, 매개변수 값 의미 x
			else
			{
				if (error == WAIT_TIMEOUT)
				{
					__debugbreak();
				}

				// 735 CP 삭제시 스레드 종료
				if (error == ERROR_ABANDONED_WAIT_0)
					return;

				// GQCS 잘못 호출
				// 이 상황 배제
				__debugbreak();
				return;
			}
		}

		// 문법상 캐스팅 후 멤버 접근
		IO_TYPE type = reinterpret_cast<OverlappedEx*>(overlapped_)->type_;

		if (type == IO_TYPE::RECV)
		{
			// 상대가 closesocket()시 (rst x) 발생
			if (numOfBytes == 0)
			{
				//__debugbreak();
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
		BOOL ret = GetQueuedCompletionStatus(sendCompletionPort_, &numOfBytes, reinterpret_cast<PULONG_PTR>(&session), &overlapped_, INFINITE);

		if (overlapped_ == nullptr && session == nullptr && numOfBytes == 0)
		{
			LOG("NETWORK", ELOG::SYSTEM_LEVEL, "Send Thread Stop");
			return;
		}

		/// TODO: 주석처리 가능한지 확인
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

				// 1236 내가 연결을 끊었을때
				// 세션 정리때 closesocket을 하므로 지금 구조에선 shutdown 호출시만 발생 가능
				if (error == ERROR_CONNECTION_ABORTED)
				{
					ReleaseSession(session);
					continue;
				}

				// 121 tcp 재전송 최대 횟수 초과시
				if (error == ERROR_SEM_TIMEOUT)
				{
					ReleaseSession(session);
					continue;
				}

				LOG("NETWORK", ELOG::ERROR_LEVEL, "send thread GQCS() error=%d / id=0x%016llx", error, session->sessionId_);
				__debugbreak();
			}
		}

		sendWorkCount_--;
		SendPost(session);
		ReleaseSession(session);
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
// WSASend() 호출 시 wsaBufcount가 0
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
