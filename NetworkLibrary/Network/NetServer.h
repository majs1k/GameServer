#pragma once
#include <string>
#include <vector>
#include <queue>
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
		uint64_t sessionId_;
		std::string ip_;
		int port_;

		//RingBuffer sendQueue_{ SEND_SIZE };
		//std::queue<Packet*> sendQueue_;
		LockFreeQueue<Packet*> sendQueue_;
		std::queue<Packet*> sendPostQueue_;

		//RingBuffer recvQueue_{ RECV_SIZE };
		Packet* recvPacket_;

		OverlappedEx sendOverlapped_;
		OverlappedEx recvOverlapped_;
		LONG ioCount_;
		LONG sendPending_;
		//char acceptBuffer[100];
		//std::recursive_mutex sessionLock_;

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
	HANDLE hNetworkIOCP_;
	std::vector<HANDLE> hWorkerThread_;
	HANDLE hAcceptThread_;
	int concurrentCount_;
	int workerCount_;

	static unsigned int __stdcall WorkerThread(void* param);
	void WorkerFunc();

	// ============================================================================
	// Session
	// ============================================================================
private:

	//std::unordered_map<__int64, Session*> sessionMap_;
	//std::recursive_mutex sessionMapLock_;
	/// TODO: 수정
	//Session** sessions_;
	Session* sessions_[20000];
	//std::vector<Session*> sessions_;
	LockFreeStack<int> sessionIndexPool_;
	std::atomic<uint64_t> idSeed_ = 0;

	constexpr static uint64_t SESSION_ID_MASK = 0x0000ffffffffffff;
	constexpr static uint64_t SESSION_INDEX_MASK = 0xffff000000000000;
	constexpr static int SESSION_INDEX_SHIFT = 48;
	constexpr static int RELEASE_MASK = 0x00010000;

protected:

	LONG sessionCount_ = 0;
	int sessionMax_;

	uint64_t AllocSessionId(Session* session);
	uint64_t GetSessionId(uint64_t sessionId);
	int GetSessionIndex(uint64_t sessionId);

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
	void CompleteDisconnect(Session* session);

	LPFN_ACCEPTEX lpfnAcceptEx_;
	LPFN_DISCONNECTEX lpfnDisconnectex_;

	// ============================================================================
	// Network
	// ============================================================================
private:

	void RecvPost(Session* session);
	void CompleteRecv(Session* session, int numOfBytes);
	void SendPost(Session* session);
	void CompleteSend(Session* session, int numOfBytes);
	void ReleaseSession(Session* session);

protected:

	bool shutdown_ = false;
	std::atomic<int> acceptCount_ = 0;
	std::atomic<int> recvMessageCount_ = 0;
	std::atomic<int> sendMessageCount_ = 0;
	std::atomic<int> acceptCountTotal_ = 0;
	std::atomic<int> disconnectCountTotal_ = 0;

	// ============================================================================
	// Send Thread
	// ============================================================================
private:

	HANDLE hSendIOCP_;
	std::vector<HANDLE> hSendThread_;

	constexpr static int SEND_CONCURRENT_COUNT = 3;
	constexpr static int SEND_WORKER_COUNT = 3;
	constexpr static int PROTOCOL_SEND_PACKET = 1;

	static unsigned int __stdcall SendThread(void* param);
	void SendFunc();

	// ============================================================================
	// Debugging
	// ============================================================================
public:

	//LockStack<int> debugIndexPool_;
	//inline static std::atomic<int> testcount = 0;
	//// playerId -> sessionId
	//std::map<int64_t, uint64_t> testset;
};
