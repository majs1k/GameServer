#pragma once
#include <string>
#include <vector>
//#include <unordered_map>
//#include <stack>
//#include <mutex>
#include <WinSock2.h>
//#include <Windows.h>

#include "../Utils/RingBuffer.h"
#include "../Utils/MemoryPoolTls.h"
#include "../Utils/LockFreeStack.h"
#include "../Utils/LockFreeQueue.h"

#define SEND_SIZE				20000
#define RECV_SIZE				20000

class Packet;

class LanServer
{
private:

	enum IO_TYPE
	{
		SEND = 0,
		RECV = 1,
	};

	struct OverlappedEx
	{
		WSAOVERLAPPED overlapped;
		IO_TYPE type;
	};

	struct Session
	{
		SOCKET socket_;
		// 고유값. sessionList의 인덱스로 활용 x (디버깅 용이)
		// 컨텐츠에서 sessionId로만 접근 (컨텐츠 구현 용이)
		uint64_t sessionId_;

		std::string ip_;
		int port_;

		//RingBuffer sendQueue_{ SEND_SIZE };
		//TRingBuffer<Packet*> sendQueue_;
		LockFreeQueue<Packet*> sendQueue_;

		TRingBuffer<Packet*> sendPostQueue_;

		//RingBuffer recvQueue_{ RECV_SIZE };
		Packet* recvPacket_;

		OverlappedEx sendOverlapped_;
		OverlappedEx recvOverlapped_;

		LONG ioCount_;
		LONG sendPending_;

		//std::recursive_mutex sessionLock_;

		Session();
		~Session();

		void Initialize(SOCKET socket, std::string ip, int port, uint64_t id);
	};

private:

	SOCKET listenSocket_;

	std::string serverIp_;
	int serverPort_;

	HANDLE hIOCP_;
	std::vector<HANDLE> hWorkerThread_;
	HANDLE hAcceptThread_;
	int concurrentCount_;
	int workerCount_;

	//std::unordered_map<__int64, Session*> sessionMap_;
	//std::recursive_mutex sessionMapLock_;

	Session* sessions[20000];

	//std::stack<int> sessionIndex_;
	LockFreeStack<int> sessionIndex_;

protected:

	LONG sessionCount_ = 0;
	int sessionMax_;

	uint64_t idSeed_ = 0;

public:

	LanServer();
	virtual ~LanServer();

	bool Start(std::string ip, int port, int sessionMax, int concurrentCount, int workerCount);
	void Stop();

	bool Disconnect(uint64_t sessionId);
	bool SendPacket(uint64_t sessionId, Packet* packet);

	// (외부/해외/공격)IP 차단 기능 + 패치 후 점검 white ip만 가능케(운영에 필수적)
	virtual bool OnConnectionRequest(const std::string& ip, int port) = 0;
	// 인자 미정
	// Session 포인터, socket -> 컨텐츠로 절대 전달 x
	virtual void OnAccept(uint64_t sessionId) = 0;
	// 연결 종료가 아닌, 세션 삭제 시 호출 (다른 사용자들에게 세션 종료를 알림)
	virtual void OnRelease(uint64_t sessionId) = 0;
	// 메세지 하나 단위로 컨텐츠에서 처리 (tcp 스트림은 컨텐츠에선 몰라야함)
	virtual void OnRecv(uint64_t sessionId, Packet* packet) = 0;

	// 컨텐츠에게 에러코드를 알려주지만 서버 끌 상황은 아닐때 (미구현)
	//virtual void OnError(int errorCode, wchar_t* str) = 0;


protected:

	bool shutdown_ = false;

	LONG acceptCount_;
	LONG recvMessageCount_;
	LONG sendMessageCount_;

public:

	void Monitoring();


private:

	static unsigned int __stdcall AcceptThread(void* param);
	static unsigned int __stdcall WorkerThread(void* param);

	void RecvPost(Session* session);
	void CompleteRecv(Session* session, int numOfBytes);

	void SendPost(Session* session);
	void CompleteSend(Session* session, int numOfBytes);

	void ReleaseSession(Session* session);

	uint64_t AllocSession();
	uint64_t GetSessionId(uint64_t sessionId);
	int GetSessionIndex(uint64_t sessionId);
};
