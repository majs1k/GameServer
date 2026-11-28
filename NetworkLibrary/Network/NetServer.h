#pragma once
#pragma comment(lib, "mysqlcppconn.lib")
#pragma comment(lib, "hiredis.lib")

#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
//#include <stack>
//#include <mutex>
#include <thread>
#include <atomic>
#include <WinSock2.h>
#include <mswsock.h>

#include "../Utils/RingBuffer.h"
#include "../Utils/MemoryPoolTls.h"
#include "../Utils/LockFreeStack.h"
#include "../Utils/LockFreeQueue.h"
#include "../Rpc/RpcDispatcher.h"

#include "../Database/MySQL.h"
#include "../Database/Redis.h"

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

	bool shutdown_ = false;

	// ============================================================================
	// IOCP
	// ============================================================================
private:

	static unsigned int __stdcall WorkerThread(void* param);
	void WorkerFunc();

	HANDLE workerCompletionPort_;
	std::vector<HANDLE> workerThread_;
	int concurrentCount_;
	int workerCount_;

	HANDLE hAcceptThread_;
	SOCKET listenSocket_;
	std::string serverIp_;
	int serverPort_;

	constexpr static int PROTOCOL_SEND_PACKET = 1;

	// ============================================================================
	// Session
	// ============================================================================
private:

	uint64_t GetSessionId(uint64_t sessionId);
	int GetSessionIndex(uint64_t sessionId);
	static void SchedulerThread(NetServer* server);
	void CheckTimeout();

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

	// ===================================================================================== //
	// Message
	// ===================================================================================== //
	/// TODO: 확인 후 이후 수정
protected:

	struct Message
	{
		uint64_t sessionId_;
		Packet* packet_;
		unsigned short protocol_;
		unsigned short type_;

		void Initialize(uint64_t sessionId, Packet* packet, unsigned short protocol, unsigned short type = 0)
		{
			sessionId_ = sessionId;
			packet_ = packet;
			protocol_ = protocol;
			type_ = type;
		}
	};

	static void LogicThread(NetServer* server)
	{
		while (!server->shutdown_)
		{
			server->ProcessMessageQueue();
		}
	}

	void ProcessMessageQueue()
	{
		while (1)
		{
			Message* message;
			bool ret = messageQueue_.Dequeue(message);

			if (ret == false)
				break;

			uint64_t sessionId = message->sessionId_;
			Packet* packet = message->packet_;
			ushort protocol = message->protocol_;
			ushort type = message->type_;

			/// TODO: 위치수정?
			procMessageCount_++;

			ProcessPacket(sessionId, packet, protocol, type);

			if (packet != nullptr)
			{
				packet->Free();
			}

			messagePool_.Free(message);
		}
	}

	void ProcessPacket(uint64_t sessionId, Packet* packet, ushort protocol, ushort type)
	{
		bool success;

		if (protocol == NETWORK_PACKET_PROTOCOL)
		{
			messageCount_CS_NETWORK_PACKET_++;
			success = dispatcher_.DispatchPacket(sessionId, packet, type);
		}
		else
		{
			success = OnRecvPacket(sessionId , packet, protocol);
		}

		if (!success)
		{
			Disconnect(sessionId);
		}
	}

	virtual bool OnRecvPacket(uint64_t sessionId, Packet* packet, ushort protocol) = 0;

	MemoryPoolTls<Message> messagePool_;
	LockFreeQueue<Message*> messageQueue_;

	std::atomic<int> procMessageCount_ = 0;
	std::atomic<int> messageCount_CS_NETWORK_PACKET_ = 0;

	std::thread logicThread_;
	RpcDispatcher dispatcher_;

	/// TODO: 수정?
	constexpr static ushort NETWORK_PACKET_PROTOCOL = 0;

	enum THREAD_TYPE
	{
		SINGLE = 0,
		MULTI = 1,
	};

	THREAD_TYPE threadType_ = THREAD_TYPE::SINGLE;

	// ============================================================================
	// Send Thread
	// ============================================================================
private:

	static unsigned int __stdcall SendThread(void* param);
	void SendFunc();

	HANDLE sendCompletionPort_;
	std::vector<HANDLE> sendThreads_;
	constexpr static int SEND_CONCURRENT_COUNT = 4;
	constexpr static int SEND_WORKER_COUNT = 4;

	// ============================================================================
	// Counter
	// ============================================================================
protected:

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
	//std::unordered_map<uint64_t, int> debuggingMap_;
	//std::mutex debuggingLock_;

	/// 로그 사용 시 동기화 문제 추적 안될 때
	//LockStack<int> debugIndexPool_;
	//inline static std::atomic<int> testcount = 0;
	//// playerId -> sessionId
	//std::map<int64_t, uint64_t> testset;

	/// 안전하지 않은 로그인 서버 더미용 함수
	std::string GetSessionIp(uint64_t sessionId);


	// ============================================================================
	// MySQL
	// ============================================================================
protected:

	void PostDbReadMessage(uint64_t sessionId, Packet* packet, unsigned short protocol)
	{
		Message* message = messagePool_.Alloc();
		message->Initialize(sessionId, packet, protocol);

		dbReadMessageCount_++;
		packet->AddRef();
		PostQueuedCompletionStatus(dbReadCompletionPort_, 0, (ULONG_PTR)message, nullptr);
	}

	MySQLTls mysql_;
	RedisTls redis_;
	std::atomic<int> dbReadMessageCount_ = 0;

private:

	/// TODO: 종료처리
	static void DbReadThread(NetServer* server)
	{
		server->DbReadFunc();
	}

	void DbReadFunc()
	{
		DWORD numOfBytes;
		Message* message;
		OVERLAPPED* overlapped_;

		while (1)
		{
			//session = nullptr;
			//numOfBytes = 0;

			// GQCS 호출 반환시 overlapped 구조체 무조건 세팅
			BOOL ret = GetQueuedCompletionStatus(dbReadCompletionPort_, &numOfBytes, reinterpret_cast<PULONG_PTR>(&message), &overlapped_, INFINITE);

			if (overlapped_ == nullptr && message == nullptr && numOfBytes == 0)
			{
				//LOG("NETWORK", ELOG::SYSTEM_LEVEL, "logic Thread Stop");
				return;
			}

			if (!ret)
			{
				__debugbreak();
			}

			uint64_t sessionId = message->sessionId_;
			Packet* packet = message->packet_;
			ushort protocol = message->protocol_;

			dbReadMessageCount_--;
			OnRecvPacket(sessionId, packet, protocol);

			if (packet != nullptr)
			{
				packet->Free();
			}

			messagePool_.Free(message);
		}
	}

	HANDLE dbReadCompletionPort_;
	std::thread dbReadThread_;
};
