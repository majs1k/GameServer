#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>

#include "Utils/Packet.h"
#include "Utils/LockFreeQueue.h"
#include "Utils/CommonUtility.h"
#include "Database/MySQL.h"

#include <windows.h>

//--------------------------------------------------------------------------------------------
// DB 저장 메시지의 헤더
//--------------------------------------------------------------------------------------------
struct st_DBQUERY_HEADER
{
	unsigned short	type_;
};


// 메시지 구조체 샘플

//--------------------------------------------------------------------------------------------
//  플레이어 레벨업
//--------------------------------------------------------------------------------------------
#define df_DBQUERY_MSG_LEVELUP			0
struct st_DBQUERY_MSG_LEVELUP
{
	__int64	playerId_;
	int	level_;
};

//--------------------------------------------------------------------------------------------
//  플레이어 돈 증가
//--------------------------------------------------------------------------------------------
#define df_DBQUERY_MSG_MONEY_ADD		1
struct st_DBQUERY_MSG_MONEY_ADD
{
	__int64	playerId_;
	int	money_;
	int	why_;		// 사유
};

//--------------------------------------------------------------------------------------------
//  퀘스트 완료
//--------------------------------------------------------------------------------------------
#define df_DBQUERY_MSG_QUEST_COMPLETE		2
struct st_DBQUERY_MSG_QUEST_COMPLETE
{
	__int64	playerId_;
	int	questId_;

	// 보상목록
};

//--------------------------------------------------------------------------------------------
//  아이템 구매
//--------------------------------------------------------------------------------------------
#define df_DBQUERY_MSG_ITEM_BUY			3
struct st_DBQUERY_MSG_ITEM_BUY
{
	__int64	playerId_;
	int	itemId_;
	int	price_;		// 만약 가격이 DB 에 정해져 있다면 넣지 않을 수도 있음
	int	slot_;

	// 아이템마다 능력치가 고정으로 정해져 있다면 ItemID 만으로 해결 되겠으나
	// 아이템마다 개별적인 추가 능력치가 생성되는 개념의 컨텐츠라면
	// 이에대한 능력치들이 모두 전달 되어야만 한다.
};

//--------------------------------------------------------------------------------------------
// 거래
//--------------------------------------------------------------------------------------------
#define df_DBQUERY_MSG_ITEM_TRADE			4
struct st_DBQUERY_MSG_ITEM_TRADE
{
	__int64	fromPlayerId_;
	__int64	fromItemSlot_;

	__int64	toPlayerId_;
	__int64	toItemSlot_;

	int	tradeMoney_;
	int	quantity_;

	// 이처럼 인벤토리의 슬롯위치만 전달하여 거래를 성사 시킬 수도 있으며
	// 아이템의 고유 ID 를 가지고 처리 하는것도 가능
};


class DBServer
{
public:

	void SetItemTradeMode(int mode)
	{
		if (mode < 1 || mode > 3)
			return;

		itemTradeMode_.store(static_cast<ItemTradeMode>(mode));
	}

public:

	void Start()
	{
		mysql_.Connect("tcp://127.0.0.1:3306", "root", "1234", "test");
		//db_.Connect("tcp://10.0.2.2:3306", "gameserver", "1234", "test");

		InitRepository();

		//MakeDBSample();

		hDbWriteIOCP_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, concurrentCount_);

		for (int i = 0; i < workerCount_; i++)
		{
			dbWriteThread_.emplace_back(WorkerThread, this);
		}

		producerThread1_ = std::thread(ProducerThread, this);
		producerThread2_ = std::thread(ProducerThread, this);

		messageEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	void Stop()
	{
		for (int i = 0; i < workerCount_; i++)
		{
			dbWriteThread_[i].join();
		}

		producerThread1_.join();
		producerThread1_.join();
	}

	void InitRepository()
	{
		/// players와 items 테이블은 분리할수밖에 없음. 안 그러면 인벤토리창 수정에 대응이 안됨.
		/// 테스트를 위해 슬롯과 아이템아이디를 같게 둠
		/// PK가 달라지면 성능에 어떤 영향을 주는지?
		//mysql_.Execute(
		//	"DROP TABLE players;"
		//);

		//mysql_.Execute(
		//	"DROP TABLE items;"
		//);

		//mysql_.Execute(
		//	"DROP TABLE quests;"
		//);


		mysql_.Execute(
			"CREATE TABLE IF NOT EXISTS players ("
			"player_id BIGINT PRIMARY KEY,"
			"money INT DEFAULT 1000000,"
			"level INT DEFAULT 1"
			")"
		);

		mysql_.Execute(
			"CREATE TABLE IF NOT EXISTS items ("
			"id BIGINT UNIQUE AUTO_INCREMENT,"
			"player_id BIGINT,"
			"slot INT,"
			"item_id INT DEFAULT 0,"
			"count INT DEFAULT 1000000,"
			"PRIMARY KEY (player_id, slot)"
			")"
		);

		mysql_.Execute(
			"CREATE TABLE IF NOT EXISTS quests ("
			"id BIGINT UNIQUE AUTO_INCREMENT,"
			"player_id BIGINT,"
			"quest_id INT,"
			"PRIMARY KEY (player_id, quest_id)"
			")"
		);
	}

	void MakeDBSample()
	{
		std::string playerQuery;
		std::string itemQuery;

		playerQuery =
			"INSERT INTO players (player_id) VALUES ";

		for (int playerIdx = 1; playerIdx <= playerMax_; playerIdx++)
		{
			if (playerIdx > 1)
				playerQuery += ", ";

			playerQuery += "(" + std::to_string(playerIdx) + ")";
		}

		itemQuery =
			"INSERT INTO items (player_id, slot, item_id) VALUES ";

		bool first = true;

		for (int playerIdx = 1; playerIdx <= playerMax_; playerIdx++)
		{
			for (int itemSlotIdx = 1; itemSlotIdx <= itemSlotMax_; itemSlotIdx++)
			{
				if (!first)
					itemQuery += ", ";

				itemQuery +=
					"(" + std::to_string(playerIdx) + ", " + std::to_string(itemSlotIdx) + ", " + std::to_string(itemSlotIdx) + ")";

				first = false;
			}
		}

		mysql_.Execute(playerQuery);
		mysql_.Execute(itemQuery);
	}

	static void ProducerThread(DBServer* server)
	{
		server->ProducerFunc();
	}

	void ProducerFunc()
	{
		while (1)
		{
			Message* message = messagePool_.Alloc();
			Packet* packet = packetPool_.Alloc();
			packet->Initialize();

			/// 테스트
			int type = 2;

			switch (type)
			{
			case df_DBQUERY_MSG_LEVELUP:
			{
				st_DBQUERY_MSG_LEVELUP* buffer = reinterpret_cast<st_DBQUERY_MSG_LEVELUP*>(packet->GetBodyPtr());

				buffer->playerId_ = RAND(1, playerMax_);
				buffer->level_ = 1;

				message->Initialize(0, packet, df_DBQUERY_MSG_LEVELUP);
			}
			break;

			case df_DBQUERY_MSG_MONEY_ADD:
			{
				st_DBQUERY_MSG_MONEY_ADD* buffer = reinterpret_cast<st_DBQUERY_MSG_MONEY_ADD*>(packet->GetBodyPtr());

				buffer->playerId_ = RAND(1, playerMax_);
				buffer->money_ = 1000;

				message->Initialize(0, packet, df_DBQUERY_MSG_MONEY_ADD);
			}
			break;

			case df_DBQUERY_MSG_QUEST_COMPLETE:
			{
				st_DBQUERY_MSG_QUEST_COMPLETE* buffer = reinterpret_cast<st_DBQUERY_MSG_QUEST_COMPLETE*>(packet->GetBodyPtr());

				buffer->playerId_ = RAND(1, playerMax_);
				buffer->questId_ = ++questIdSeed_;

				message->Initialize(0, packet, df_DBQUERY_MSG_QUEST_COMPLETE);
			}
			break;

			case df_DBQUERY_MSG_ITEM_BUY:
			{
				st_DBQUERY_MSG_ITEM_BUY* buffer = reinterpret_cast<st_DBQUERY_MSG_ITEM_BUY*>(packet->GetBodyPtr());

				buffer->playerId_ = RAND(1, playerMax_);

				int itemId = RAND(1, itemSlotMax_);

				buffer->itemId_ = itemId;
				buffer->price_ = RAND(1, 100);
				buffer->slot_ = itemId;

				message->Initialize(0, packet, df_DBQUERY_MSG_ITEM_BUY);
			}
			break;

			case df_DBQUERY_MSG_ITEM_TRADE:
			{
				st_DBQUERY_MSG_ITEM_TRADE* buffer = reinterpret_cast<st_DBQUERY_MSG_ITEM_TRADE*>(packet->GetBodyPtr());

				int id1 = RAND(1, playerMax_);
				int id2;
				int itemSlot = RAND(1, itemSlotMax_);

				while (1)
				{
					id2 = RAND(1, playerMax_);

					if (id2 != id1)
						break;
				}

				buffer->fromPlayerId_ = id1;
				buffer->fromItemSlot_ = itemSlot;
				buffer->toPlayerId_ = id2;
				buffer->toItemSlot_ = itemSlot;
				buffer->tradeMoney_ = RAND(1, 50);
				buffer->quantity_ = RAND(1, 50);

				message->Initialize(0, packet, df_DBQUERY_MSG_ITEM_TRADE);
			}
			break;


			default:

				break;
			}

			messageCount_++;
			PostQueuedCompletionStatus(hDbWriteIOCP_, 0, (ULONG_PTR)message, nullptr);
			//messageQueue_.Enqueue(message);

			//SetEvent(messageEvent_);

			/// 테스트
			Sleep(1);
		}
	}


	static void WorkerThread(DBServer* server)
	{
		server->WorkerFunc();

		//while (1)
		//{
		//	WaitForSingleObject(server->messageEvent_, INFINITE);

		//	while (1)
		//	{
		//		Message* message;

		//		bool b = server->messageQueue_.Dequeue(message);
		//		if (b == false)
		//			break;

		//		server->DbWriteProc(message);
		//	}
		//}
	}

	void WorkerFunc()
	{
		DWORD numOfBytes;
		Message* message;
		OVERLAPPED* overlapped_;

		while (1)
		{
			//session = nullptr;
			//numOfBytes = 0;

			// GQCS 호출 반환시 overlapped 구조체 무조건 세팅
			BOOL ret = GetQueuedCompletionStatus(hDbWriteIOCP_, &numOfBytes, reinterpret_cast<PULONG_PTR>(&message), &overlapped_, INFINITE);

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

			DbWriteProc(message);
		}
	}

	void DbWriteProc(Message* message)
	{
		Packet* packet = message->packet_;
		ushort protocol = message->protocol_;

		messageCount_--;

		switch (protocol)
		{
		case df_DBQUERY_MSG_LEVELUP:
		{
			st_DBQUERY_MSG_LEVELUP* pkt = reinterpret_cast<st_DBQUERY_MSG_LEVELUP*>(packet->GetBodyPtr());

			int64_t playerId = pkt->playerId_;
			int level = pkt->level_;

			std::string query =
				"UPDATE players SET level = level + " + std::to_string(level) +
				" WHERE player_id = " + std::to_string(playerId);

			mysql_.Execute(query);

			break;
		}

		case df_DBQUERY_MSG_MONEY_ADD:
		{
			st_DBQUERY_MSG_MONEY_ADD* pkt = reinterpret_cast<st_DBQUERY_MSG_MONEY_ADD*>(packet->GetBodyPtr());

			int64_t playerId = pkt->playerId_;
			int money = pkt->money_;
			int why = pkt->why_;

			for (int i = 0; i < 10; i++)
			{
				std::string query =
					"UPDATE players SET money = money + " + std::to_string(money) +
					" WHERE player_id = " + std::to_string(playerId);

				mysql_.Execute(query);
			}


			break;
		}

		case df_DBQUERY_MSG_QUEST_COMPLETE:
		{
			st_DBQUERY_MSG_QUEST_COMPLETE* pkt = reinterpret_cast<st_DBQUERY_MSG_QUEST_COMPLETE*>(packet->GetBodyPtr());

			int64_t playerId = pkt->playerId_;
			int questId = pkt->questId_;

			for (int i = 0; i < 10; i++)
			{
				std::string query =
					"INSERT INTO quests (player_id, quest_id) VALUES (" +
					std::to_string(playerId) + ", " + std::to_string(questId) + ")";

				mysql_.Execute(query);
			}


			break;
		}

		case df_DBQUERY_MSG_ITEM_BUY:
		{
			st_DBQUERY_MSG_ITEM_BUY* pkt = reinterpret_cast<st_DBQUERY_MSG_ITEM_BUY*>(packet->GetBodyPtr());

			int64_t playerId = pkt->playerId_;
			int itemId = pkt->itemId_;
			int price = pkt->price_;
			int slot = pkt->slot_;

			std::string query =
				"BEGIN;"
				"UPDATE items SET count = count + 1 "
				"WHERE player_id = " + std::to_string(playerId) + " AND slot = " + std::to_string(slot) + ";";

			query +=
				"UPDATE players SET money = money - " + std::to_string(price) +
				" WHERE player_id = " + std::to_string(playerId) + ";"
				+ "COMMIT";

			mysql_.ExecuteMulti(query);

			break;
		}

		case df_DBQUERY_MSG_ITEM_TRADE:
		{
			st_DBQUERY_MSG_ITEM_TRADE* pkt =
				reinterpret_cast<st_DBQUERY_MSG_ITEM_TRADE*>(packet->GetBodyPtr());

			int64_t fromPlayerId = pkt->fromPlayerId_;
			int64_t fromItemSlot = pkt->fromItemSlot_;
			int64_t toPlayerId = pkt->toPlayerId_;
			int64_t toItemSlot = pkt->toItemSlot_;
			int tradeMoney = pkt->tradeMoney_;
			int quantity = pkt->quantity_;

			ProcessItemTrade(fromPlayerId, fromItemSlot, toPlayerId, toItemSlot, tradeMoney, quantity);

			break;
		}

		default:
			__debugbreak();
		}

		dbWriteCount_++;

		if (packet != nullptr)
		{
			packet->Free();
		}

		messagePool_.Free(message);

	}

	void ProcessItemTrade(int64_t fromPlayerId, int fromItemSlot, int64_t toPlayerId, int toItemSlot, int tradeMoney, int quantity)
	{
		ItemTradeMode mode = itemTradeMode_.load();

		switch (mode)
		{
		case ItemTradeMode::Single:
		{
			mysql_.Execute(
				"BEGIN;"
			);

			// 보내는 사람 아이템 감소
			mysql_.Execute(
				"UPDATE items SET count = count - "
				+ std::to_string(quantity) +
				" WHERE player_id = " + std::to_string(fromPlayerId) +
				" AND slot = " + std::to_string(fromItemSlot)
			);


			// 받는 사람 아이템 증가
			mysql_.Execute(
				"UPDATE items SET count = count + "
				+ std::to_string(quantity) +
				" WHERE player_id = " + std::to_string(toPlayerId) +
				" AND slot = " + std::to_string(toItemSlot)
			);


			// 보내는 사람 돈 증가
			mysql_.Execute(
				"UPDATE players SET money = money + "
				+ std::to_string(tradeMoney) +
				" WHERE player_id = " + std::to_string(fromPlayerId)
			);


			// 받는 사람 돈 감소
			mysql_.Execute(
				"UPDATE players SET money = money - "
				+ std::to_string(tradeMoney) +
				" WHERE player_id = " + std::to_string(toPlayerId)
			);

			mysql_.Execute(
				"COMMIT;"
			);
		}
		break;

		case ItemTradeMode::Multi:
		{
			std::string query;

			//query += "START TRANSACTION;";

			// 보내는 사람 아이템 감소
			query +=
				"UPDATE items SET count = count - " +
				std::to_string(quantity) +
				" WHERE player_id = " + std::to_string(fromPlayerId) +
				" AND slot = " + std::to_string(fromItemSlot) + ";";

			// 받는 사람 아이템 증가
			query +=
				"UPDATE items SET count = count + " +
				std::to_string(quantity) +
				" WHERE player_id = " + std::to_string(toPlayerId) +
				" AND slot = " + std::to_string(toItemSlot) + ";";

			// 보내는 사람 돈 증가
			query +=
				"UPDATE players SET money = money + " +
				std::to_string(tradeMoney) +
				" WHERE player_id = " + std::to_string(fromPlayerId) + ";";

			// 받는 사람 돈 감소
			query +=
				"UPDATE players SET money = money - " +
				std::to_string(tradeMoney) +
				" WHERE player_id = " + std::to_string(toPlayerId) + ";";

			//query += "COMMIT;";

			mysql_.ExecuteMulti(query);
		}
		break;

		case ItemTradeMode::MultiTransaction:
		{
			std::string query;

			query += "START TRANSACTION;";

			// 보내는 사람 아이템 감소
			query +=
				"UPDATE items SET count = count - " +
				std::to_string(quantity) +
				" WHERE player_id = " + std::to_string(fromPlayerId) +
				" AND slot = " + std::to_string(fromItemSlot) + ";";

			// 받는 사람 아이템 증가
			query +=
				"UPDATE items SET count = count + " +
				std::to_string(quantity) +
				" WHERE player_id = " + std::to_string(toPlayerId) +
				" AND slot = " + std::to_string(toItemSlot) + ";";

			// 보내는 사람 돈 증가
			query +=
				"UPDATE players SET money = money + " +
				std::to_string(tradeMoney) +
				" WHERE player_id = " + std::to_string(fromPlayerId) + ";";

			// 받는 사람 돈 감소
			query +=
				"UPDATE players SET money = money - " +
				std::to_string(tradeMoney) +
				" WHERE player_id = " + std::to_string(toPlayerId) + ";";

			query += "COMMIT;";

			mysql_.ExecuteMulti(query);
		}
		break;

		default:

			__debugbreak();
		}
	}

	void Monitoring()
	{
		int dbWriteCount = dbWriteCount_.exchange(0);

		ItemTradeMode mode = itemTradeMode_.load();

		const char* modeName = "";

		switch (mode)
		{
		case ItemTradeMode::Single:
			modeName = "Single";
			break;

		case ItemTradeMode::Multi:
			modeName = "Multi";
			break;

		case ItemTradeMode::MultiTransaction:
			modeName = "Multi + Transaction";
			break;
		}

		printf("=========================\n");
		printf("=========================\n\n");

		printf("messageCount       : %d\n", messageCount_.load());
		printf("dbWriteTPS         : %d\n", dbWriteCount);
		printf("Mode			   : %d %s\n\n", mode, modeName);
	}

private:

	struct Message
	{
		uint64_t sessionId_;
		Packet* packet_;
		ushort protocol_;

		void Initialize(uint64_t sessionId, Packet* packet, ushort protocol)
		{
			sessionId_ = sessionId;
			packet_ = packet;
			protocol_ = protocol;
		}
	};

	HANDLE hDbWriteIOCP_;
	std::vector<std::thread> dbWriteThread_;
	int concurrentCount_ = 1;
	int workerCount_ = 1;

	MemoryPoolTls<Message> messagePool_;
	LockFreeQueue<Message*> messageQueue_;
	HANDLE messageEvent_;

	std::thread producerThread1_;
	std::thread producerThread2_;

	std::atomic<int> messageCount_ = 0;
	std::atomic<int> dbWriteCount_ = 0;

	constexpr static int playerMax_ = 100000;
	//constexpr static int itemMax_ = 10;
	constexpr static int itemSlotMax_ = 10;
	std::atomic<int> questIdSeed_ = 0;

private:

	MySQL mysql_;

	enum class ItemTradeMode
	{
		Single = 1,
		Multi = 2,
		MultiTransaction = 3
	};

	std::atomic<ItemTradeMode> itemTradeMode_ = ItemTradeMode::Single;
};


#pragma comment(lib, "winmm.lib")

int main()
{
	timeBeginPeriod(1);

	DBServer server;
	server.Start();

	while (1)
	{
		if (GetAsyncKeyState(VK_ESCAPE) & 0x0001)
		{
			__debugbreak();
			break;
		}

		if (GetAsyncKeyState('1') & 0x0001)
		{
			server.SetItemTradeMode(1);
		}

		if (GetAsyncKeyState('2') & 0x0001)
		{
			server.SetItemTradeMode(2);
		}

		if (GetAsyncKeyState('3') & 0x0001)
		{
			server.SetItemTradeMode(3);
		}

		server.Monitoring();

		Sleep(1000);
	}

	return 0;
}
