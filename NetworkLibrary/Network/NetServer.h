#pragma once
#include <string>
#include <vector>
//#include <unordered_map>
//#include <stack>
//#include <mutex>
#include <atomic>
#include <WinSock2.h>
//#include <Windows.h>
#include <mswsock.h>

#include "../Utils/RingBuffer.h"
#include "../Utils/MemoryPoolTls.h"
#include "../Utils/LockFreeStack.h"
#include "../Utils/LockFreeQueue.h"

#include "../Utils/LockQueue.h"
#include <map>

#define SEND_SIZE				20000
#define RECV_SIZE				200

class Packet;

class NetServer
{
private:

	enum IO_TYPE
	{
		SEND = 0,
		RECV = 1,
		ACCEPT = 2,
		DISCONNECT = 3,
	};

	struct OverlappedEx
	{
		WSAOVERLAPPED overlapped_;
		IO_TYPE type_;
	};

	struct Session
	{
		//OverlappedEx acceptOverlapped_;
		//OverlappedEx disconnectOverlapped_;

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

		RingBuffer recvQueue_{ RECV_SIZE };
		//Packet* recvPacket_;

		OverlappedEx sendOverlapped_;
		OverlappedEx recvOverlapped_;

		LONG ioCount_;
		LONG sendPending_;

		//std::recursive_mutex sessionLock_;
		char acceptBuffer[100];

		Session();
		~Session();

		void Initialize(uint64_t sessionId, std::string& ip, int port);
		void Clear();
	};

public:

	NetServer();
	virtual ~NetServer();

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


private:

	SOCKET listenSocket_;
	std::string serverIp_;
	int serverPort_;

	HANDLE hNetworkIOCP_;
	std::vector<HANDLE> hWorkerThread_;
	HANDLE hAcceptThread_;
	int concurrentCount_;
	int workerCount_;

	//std::unordered_map<__int64, Session*> sessionMap_;
	//std::recursive_mutex sessionMapLock_;

	/// TODO: 수정
	//Session** sessions_;
	Session* sessions_[20000];

	//std::stack<int> sessionIndexPool_;
	LockFreeStack<int> sessionIndexPool_;

protected:

	LONG sessionCount_ = 0;
	int sessionMax_;
	std::atomic<uint64_t> idSeed_ = 0;

protected:

	bool shutdown_ = false;
	std::atomic<int> acceptCount_ = 0;
	std::atomic<int> recvMessageCount_ = 0;
	std::atomic<int> sendMessageCount_ = 0;
	std::atomic<int> acceptCountTotal_ = 0;
	std::atomic<int> disconnectCountTotal_ = 0;

private:

	static unsigned int __stdcall WorkerThread(void* param);
	void WorkerFunc();

	constexpr static uint64_t SESSION_ID_MASK = 0x0000ffffffffffff;
	constexpr static uint64_t SESSION_INDEX_MASK = 0xffff000000000000;
	constexpr static int SESSION_INDEX_SHIFT = 48;
	constexpr static int RELEASE_MASK = 0x00010000;

private:
	// ============================================================================
	// Accept Thread
	// ============================================================================
	static unsigned int __stdcall AcceptThread(void* param);
	void AcceptFunc();
	void CompleteAccept(SOCKET socket);

	// ============================================================================
	// AcceptEx
	// ============================================================================
	void AcceptPost(Session* session);
	void CompleteAccept(Session* session);
	void CompleteDisconnect(Session* session);

	LPFN_ACCEPTEX lpfnAcceptEx_;
	LPFN_DISCONNECTEX lpfnDisconnectex_;

	// ============================================================================
	// Network
	// ============================================================================
	void RecvPost(Session* session);
	void CompleteRecv(Session* session, int numOfBytes);
	void SendPost(Session* session);
	void CompleteSend(Session* session, int numOfBytes);
	void ReleaseSession(Session* session);

protected:

	// 세션의 배열 인덱스가 아이디에 포함된 이유 -> 컨텐츠에서 id로만 접근하기에
	uint64_t AllocSessionId(Session* session);
	uint64_t GetSessionId(uint64_t sessionId);
	int GetSessionIndex(uint64_t sessionId);

	// ============================================================================
	// Send Thread
	// ============================================================================

private:

	HANDLE hSendIOCP_;
	std::vector<HANDLE> hSendThread_;

	constexpr static int SEND_CONCURRENT_COUNT = 3;
	constexpr static int SEND_WORKER_COUNT = 3;
	
	constexpr static int SEND_MESSAGE_NUM = 1;

	static unsigned int __stdcall SendThread(void* param);
	void SendFunc();


	// ============================================================================
	// 디버깅
	// ============================================================================

//public:
//	LockStack<int> debugIndexPool_;
//	inline static std::atomic<int> testcount = 0;
//	// playerId -> sessionId
//	std::map<int64_t, uint64_t> testset;
};
