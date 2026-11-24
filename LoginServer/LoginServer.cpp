#include "LoginServer.h"
#include "RpcModule.h"
#include "Utils/Logger.h"
#include "Utils/Profiler.h"


void LoginServer::Message::Initialize(uint64_t sessionId, Packet* packet, ushort protocol)
{
	sessionId_ = sessionId;
	packet_ = packet;
	protocol_ = protocol;
}

LoginServer::LoginServer()
{
	/// TODO: config
	/// DB
	db_.Connect("tcp://127.0.0.1:3306", "root", "1234", "test");
	//db_.Connect("tcp://10.0.2.2:3306", "gameserver", "1234", "test");

	InitRepository();

	/// TODO: 
	//MakeDBSample();


	g_RpcProxy.serverProxy_ = this;

	for (int i = 0; i < LOGIC_THREAD_NUM; i++)
	{
		logicThreads_.emplace_back(std::thread(LogicThread, this)) ;
	}

	//schedulerThread_ = std::thread(SchedulerThread, this);
	monitorThread_ = std::thread(MonitorThread, this);
}

LoginServer::~LoginServer()
{
	for (int i = 0; i < LOGIC_THREAD_NUM; i++)
	{
		logicThreads_[i].join();
	}

	//schedulerThread_.join();
	monitorThread_.join();
}

bool LoginServer::OnConnectionRequest(const std::string& ip, int port)
{
	return true;
}

void LoginServer::OnAccept(uint64_t sessionId)
{
	//CreatePlayer(sessionId);

	/// 타이머 등록 (Disconnect)
	/// 우선순위 큐에 삽입

}

void LoginServer::OnRelease(uint64_t sessionId)
{
	//RemovePlayer(sessionId);
}

void LoginServer::OnRecv(uint64_t sessionId, Packet* packet)
{
	// 로직 스레드에서 패킷 처리
	// IO 스레드에서 처리하도록 수정 x (컨텐츠에서 동기화 문제 발생 가능)
	Message* message = messagePool_.Alloc();
	message->Initialize(sessionId, packet, CS_NETWORK_PACKET_LOGIN_SERVER);
	packet->AddRef();

	//messageQueue_.Enqueue(message);
	messageCount_++;
	PostQueuedCompletionStatus(logicCompletionPort, 0, (ULONG_PTR)message, nullptr);
}

void LoginServer::LogicThread(LoginServer* server)
{
	server->LogicFunc();
}

void LoginServer::LogicFunc()
{
	DWORD numOfBytes;
	Message* message;
	OVERLAPPED* overlapped_;

	while (1)
	{
		//session = nullptr;
		//numOfBytes = 0;

		// GQCS 호출 반환시 overlapped 구조체 무조건 세팅
		BOOL ret = GetQueuedCompletionStatus(logicCompletionPort, &numOfBytes, reinterpret_cast<PULONG_PTR>(&message), &overlapped_, INFINITE);

		if (overlapped_ == nullptr && message == nullptr && numOfBytes == 0)
		{
			return;
		}

		if (!ret)
		{
			int error = GetLastError();

			// 실패한 IO (연결 끊김)
			// 완료 패킷을 꺼내서, 실패한 IO에 대한 정보를 매개변수에 저장함
			if (overlapped_ != nullptr)
			{


				__debugbreak();
				continue;
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

		ProcessMessage(message);
	}
}


void LoginServer::MonitorThread(LoginServer* server)
{
	while (!server->shutdown_)
	{
		server->Monitoring();

		Sleep(1'000);
	}
}


void LoginServer::Monitoring()
{
	int acceptCount = acceptCount_.exchange(0);

	int procMessgaeCount = procMessageCount_.exchange(0);
	int recvMessageCount = recvMessageCount_.exchange(0);
	int sendMessageCount = sendMessageCount_.exchange(0);

	int messageCount_NETWORK_PACKET = messageCount_CS_NETWORK_PACKET_.exchange(0);

	int messageCount_CS_ReqLogin = messageCount_CS_ReqLogin_.exchange(0);
	int messageCount_CS_ReqSectorMove = messageCount_CS_ReqSectorMove_.exchange(0);
	int messageCount_CS_ReqChat = messageCount_CS_ReqChat_.exchange(0);


	printf("=========================\n");
	printf("=========================\n\n");

	printf("Session	   : %d\n", sessionCount_);

	printf("Accept     : %d\n", acceptCountTotal_.load());
	printf("Discon     : %d\n", disconnectCountTotal_.load());
	printf("Timeout    : %d\n\n", timeoutCountTotal_.load());

	printf("Accept TPS : %d\n", acceptCount);
	printf("MessageCnt : %d\n", messageCount_.load());
	printf("SendWork   : %d\n\n", sendWorkCount_.load());

	printf("Update TPS : %d\n", procMessgaeCount);
	printf("Recv   TPS : %d\n", recvMessageCount);
	printf("Send   TPS : %d\n\n", sendMessageCount);

	printf("Network    : %d\n", messageCount_NETWORK_PACKET);
	printf("Login      : %d\n", messageCount_CS_ReqLogin);
	printf("Move       : %d\n", messageCount_CS_ReqSectorMove);
	printf("Chat       : %d\n\n", messageCount_CS_ReqChat);

	printf("Packet  Pool : %d\n", packetPool_.Capacity());
	printf("Message Pool : %d\n", messagePool_.Capacity());
}


void LoginServer::InitRepository()
{
	try
	{
		/// TODO: 수정
		db_.Execute(
			"DROP TABLE users;"
		);

		db_.Execute(
			"CREATE TABLE IF NOT EXISTS users ("
			"user_id BIGINT PRIMARY KEY,"
			"email VARCHAR(100) NOT NULL UNIQUE,"
			"nickname VARCHAR(100) NOT NULL UNIQUE,"
			"created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
			"updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
			"ON UPDATE CURRENT_TIMESTAMP"
			"last_login_at TIMESTAMP"
			")"
		);
	}
	catch (sql::SQLException& e)
	{
		std::cout << "SQL Error: " << e.what() << '\n';
		std::cout << "Error Code: " << e.getErrorCode() << '\n';
		std::cout << "SQL State: " << e.getSQLState() << '\n';

		__debugbreak();
	}
}
