#pragma once
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
//#include <stack>
//#include <mutex>
#include <thread>
#include <atomic>
#include <WinSock2.h>
//#include <Windows.h>
#include <mswsock.h>

#include "../Utils/RingBuffer.h"
#include "../Utils/MemoryPoolTls.h"
#include "../Utils/LockFreeStack.h"
#include "../Utils/LockFreeQueue.h"

// HACK: 헤더 변경시 수정
#define SEND_SIZE				1000
#define RECV_SIZE				500

class Packet;

class NetServer
{
private:

	enum IO_TYPE
	{
		SEND = 0,
		RECV = 1,
		ACCEPT = 2,
	};

	struct OverlappedEx
	{
		WSAOVERLAPPED overlapped_;
		IO_TYPE type_;
	};

	// 클라이언트 세션
	struct Session
	{
		//OverlappedEx acceptOverlapped_;
		SOCKET socket_;
		uint64_t sessionId_;
		std::string ip_;
		int port_;

		//RingBuffer sendQueue_{ SEND_SIZE };
		//std::queue<Packet*> sendQueue_;
		LockFreeQueue<Packet*> sendQueue_;
		std::queue<Packet*> sendPostQueue_;

		RingBuffer recvQueue_{ RECV_SIZE };
		//Packet* recvPacket_;

		OverlappedEx sendOverlapped_;
		OverlappedEx recvOverlapped_;
		LONG ioCount_;
		LONG sendPending_;
		//char acceptBuffer[100];
		//std::recursive_mutex sessionLock_;

		uint64_t lastRecvTime_;

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

protected:

	virtual bool OnConnectionRequest(const std::string& ip, int port) = 0;
	virtual void OnAccept(uint64_t sessionId) = 0;
	virtual void OnRelease(uint64_t sessionId) = 0;
	virtual void OnRecv(uint64_t sessionId, Packet* packet) = 0;

	// ============================================================================
	// IOCP
	// ============================================================================
private:

	SOCKET listenSocket_;
	std::string serverIp_;
	int serverPort_;
	HANDLE workerCompletionPort_;
	std::vector<HANDLE> workerThread_;
	HANDLE hAcceptThread_;
	int concurrentCount_;
	int workerCount_;

	static unsigned int __stdcall WorkerThread(void* param);
	void WorkerFunc();

	constexpr static int PROTOCOL_SEND_PACKET = 1;

	// ============================================================================
	// Session
	// ============================================================================
private:

	//std::unordered_map<__int64, Session*> sessionMap_;
	//std::recursive_mutex sessionMapLock_;
	std::vector<Session*> sessions_;
	LockFreeStack<int> sessionIndexPool_;
	std::atomic<uint64_t> idSeed_ = 0;

	constexpr static uint64_t SESSION_ID_MASK = 0x0000ffffffffffff;
	constexpr static uint64_t SESSION_INDEX_MASK = 0xffff000000000000;
	constexpr static int SESSION_INDEX_SHIFT = 48;
	constexpr static int RELEASE_MASK = 0x00010000;

protected:

	LONG sessionCount_ = 0;
	int sessionMax_;
	std::thread schedulerThread_;
	constexpr static int TIMEOUT_MS = 40'000;

	uint64_t GetSessionId(uint64_t sessionId);
	int GetSessionIndex(uint64_t sessionId);
	static void SchedulerThread(NetServer* server);
	void CheckTimeout();

	// ============================================================================
	// Accept Thread
	// ============================================================================
private:

	static unsigned int __stdcall AcceptThread(void* param);
	void AcceptFunc();
	void CompleteAccept(SOCKET socket);

	// ============================================================================
	// AcceptEx
	// ============================================================================
private:

	void AcceptPost(Session* session);
	void CompleteAccept(Session* session);

	LPFN_ACCEPTEX lpfnAcceptEx_;
	//LPFN_DISCONNECTEX lpfnDisconnectex_;

	// ============================================================================
	// Network
	// ============================================================================
private:

	void RecvPost(Session* session);
	void CompleteRecv(Session* session, int numOfBytes);
	void SendPost(Session* session);
	void CompleteSend(Session* session, int numOfBytes);
	void ReleaseSession(Session* session);

	// ============================================================================
	// Send Thread
	// ============================================================================
private:

	HANDLE sendCompletionPort_;
	std::vector<HANDLE> sendThreads_;

	constexpr static int SEND_CONCURRENT_COUNT = 4;
	constexpr static int SEND_WORKER_COUNT = 4;

	static unsigned int __stdcall SendThread(void* param);
	void SendFunc();

	// ============================================================================
	// Counter
	// ============================================================================
protected:

	bool shutdown_ = false;
	std::atomic<int> acceptCount_ = 0;
	std::atomic<int> recvMessageCount_ = 0;
	std::atomic<int> sendMessageCount_ = 0;
	std::atomic<int> acceptCountTotal_ = 0;
	std::atomic<int> disconnectCountTotal_ = 0;
	std::atomic<int> sendWorkCount_ = 0;
	std::atomic<int> timeoutCountTotal_ = 0;

	// ============================================================================
	// Debugging
	// ============================================================================
public:

	/// 더미에서 attack 안 켰을때 연결 종료 전 = 메세지 도착으로 확인 가능
	std::unordered_map<uint64_t, int> debuggingMap_;
	std::mutex debuggingLock_;

	/// 로그 사용 시 동기화 문제 추적 안될 때
	//LockStack<int> debugIndexPool_;
	//inline static std::atomic<int> testcount = 0;
	//// playerId -> sessionId
	//std::map<int64_t, uint64_t> testset;

	/// 안전하지 않은 로그인 서버 전용 함수
	std::string GetSessionIp(uint64_t sessionId)
	{
		Session* session = sessions_[GetSessionIndex(sessionId)];
		return session->ip_;
	}
};
