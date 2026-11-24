#pragma once
//#include <list>
#include <queue>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include "Network/NetServer.h"
#include "Utils/Packet.h"
#include "Utils/LockFreeQueue.h"
#include "Utils/MemoryPool.h"
#include "Database/Database.h"

#include "CommonProtocol.h"
#include "RPC/RpcStub.h"


class LoginServer : public NetServer, public RpcStub
{
private:

	//struct Player
	//{
	//	uint64_t sessionId_;
	//	int64_t playerId_;
	//	LoginId loginId_;
	//	Nickname nickname_;
	//	bool isLogin_;
	//	bool isInitSector_;
	//	uint64_t lastRecvTime_;
	//	// 0 ~ 49
	//	unsigned short sectorX_;
	//	unsigned short sectorY_;

	//	void Initialize(int64_t playerId, LoginId& loginId, Nickname& nickname);
	//	void Initialize(uint64_t sessionId);
	//};

	struct Message
	{
		uint64_t sessionId_;
		Packet* packet_;
		ushort protocol_;

		void Initialize(uint64_t sessionId, Packet* packet, ushort protocol);
	};

	enum PROTOCOL
	{
		CS_NETWORK_PACKET_CHAT_SERVER = 0,
		SS_CREATE_PLAYER = 1,
		SS_REMOVE_PLAYER = 2,
		SS_CHECK_TIMEOUT = 3,

		CS_NETWORK_PACKET_LOGIN_SERVER = 100,
		IN_SESSION_TIMEOUT = 101,
	};

public:

	LoginServer();
	~LoginServer();

	// ===================================================================================== //
	// NetServer virtual func
	// ===================================================================================== //
private:

	bool OnConnectionRequest(const std::string& ip, int port) override;
	void OnAccept(uint64_t sessionId) override;
	void OnRelease(uint64_t sessionId) override;
	void OnRecv(uint64_t sessionId, Packet* packet) override;

	// ===================================================================================== //
	// Thraed
	// ===================================================================================== //
private:

	//static void LogicThread(LoginServer* server);
	//static void SchedulerThread(LoginServer* server);
	static void MonitorThread(LoginServer* server);
	void Monitoring();

	constexpr static int LOGIC_THREAD_NUM = 4;
	std::vector<std::thread> logicThreads_;

	//std::thread schedulerThread_;
	std::thread monitorThread_;

	// ===================================================================================== //
	// Message & Player
	// ===================================================================================== //
private:

	MemoryPoolTls<Message> messagePool_;
	//LockFreeQueue<Message*> messageQueue_;
	std::atomic<int> messageCount_ = 0;
	HANDLE logicCompletionPort;


	//MemoryPool<Player> playerPool_;
	//std::unordered_map<uint64_t, Player*> sessionIdToPlayer_;
	//std::unordered_map<int64_t, Player*> playerMap_;
	//int playerCount_ = 0;

	//constexpr static int SECTOR_WIDTH = 50;
	//constexpr static int SECTOR_HEIGHT = 50;
	//constexpr static int dx[3] = { 0, 1, -1 };
	//constexpr static int dy[3] = { 0, 1, -1 };

	//std::list<Player*> sector_[SECTOR_HEIGHT][SECTOR_WIDTH];


	static void LogicThread(LoginServer* server);

	void LogicFunc();

	// ===================================================================================== //
	// Protocol
	// ===================================================================================== //
private:

	void ProcessMessage(Message* message);
	/// 로그인 과정을 스케일아웃으로 구현할 거라 플레이어는 구현 안함
	/// 해당 세션이 아무 스레드에서 실행 -> 가능?
	//void CreatePlayer(uint64_t sessionId);
	//void RemovePlayer(uint64_t sessionId);
	void CheckTimeout();
	//bool SS_CreatePlayer(uint64_t sessionId);
	//bool SS_RemovePlayer(uint64_t sessionId);
	bool SS_CheckTimeout();

	/// 수정
	constexpr static int TIMEOUT_MS = 40'000;
	std::atomic<int> procMessageCount_ = 0;
	std::atomic<int> messageCount_CS_NETWORK_PACKET_ = 0;
	//std::atomic<int> messageCount_SS_CreatePlayer_ = 0;
	//std::atomic<int> messageCount_SS_RemovePlayer_ = 0;
	std::atomic<int> timeoutCountTotal_ = 0;

	// ===================================================================================== //
	// RPC
	// ===================================================================================== //
private:

	bool CS_ReqLogin(uint64_t sessionId, int64_t userId, SessionKey& sessionKey) override;
	/// 플레이어가 없다면 하트비트는 없음
	//bool CS_ReqHeartbeat(uint64_t sessionId) override;

	// ===================================================================================== //
	// DB
	// ===================================================================================== //
private:

	/// 스마트포인터
	Database db_;

public:

	void InitRepository();

	//std::mutex mtx;
	//std::condition_variable cv;
	//bool ready = false;

	std::thread timerThread_;
	std::queue<Message*> timerQueue_;

	/// 단조 증가 시계
	using Clock = std::chrono::steady_clock;
	using TimePoint = Clock::time_point;

	struct A
	{
		TimePoint lastRecvTime_ = std::chrono::steady_clock::now();;
	};


	void Timer()
	{
		while (1)
		{
			//std::unique_lock<std::mutex> lock(mtx);

			//bool result = cv.wait_for(lock,std::chrono::seconds(3),
			//	[this] { return ready; }
			//);

			//Sleep(30);
			std::this_thread::sleep_for(std::chrono::seconds(30));


			auto now = Clock::now();
			auto future = now + std::chrono::seconds(3);

			if (Clock::now() >= future)
			{
				// 3초 경과
			}



			if (timerQueue_.empty())
				continue;

			Message* message = timerQueue_.front();

			/// + if 현재시간이 메세지의 시간보다 지낫다면
			timerQueue_.pop();

			ProcessMessage(message);

			//Disconnect(sessionId);
			
		}
	}
};

