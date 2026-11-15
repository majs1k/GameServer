#include "NetServer.h"
#include "../Utils/Packet.h"
#include "../Utils/Logger.h"
#include "../Utils/Profiler.h"
#include <Windows.h>

void NetServer::RecvPost(Session* session)
{
	ZeroMemory(&session->recvOverlapped_.overlapped_, sizeof(WSAOVERLAPPED));

	WSABUF wsaBuf[2];

	wsaBuf[0].buf = session->recvQueue_.GetRearBufferPtr();
	wsaBuf[0].len = session->recvQueue_.DirectEnqueueSize();

	DWORD flags = 0;
	int retval;

	if (session->recvQueue_.FreeSize() == session->recvQueue_.DirectEnqueueSize())
	{
		// buf 1개
		retval = WSARecv(session->socket_, wsaBuf, 1, nullptr, &flags, (WSAOVERLAPPED*)&session->recvOverlapped_, NULL);
	}
	else
	{
		// buf 2개
		wsaBuf[1].buf = session->recvQueue_.GetBufferPtr();
		wsaBuf[1].len = session->recvQueue_.FreeSize() - session->recvQueue_.DirectEnqueueSize();

		retval = WSARecv(session->socket_, wsaBuf, 2, nullptr, &flags, (WSAOVERLAPPED*)&session->recvOverlapped_, NULL);
	}

	if (retval == SOCKET_ERROR)
	{
		int error = WSAGetLastError();

		if (error == ERROR_IO_PENDING)
		{
			//printf("recv() Direct IO\n");
		}
		else if (error == WSAECONNRESET || error == WSAESHUTDOWN)
		{
			ReleaseSession(session);

			return;
		}
		// 10038 내가 연결을 끊었을때 (closesocket)
		// 세션 정리때 소켓을 닫는 구조라 이 에러가 정상적인 상황에선 불가능
		else if (error == WSAENOTSOCK)
		{
			__debugbreak();
			ReleaseSession(session);

			return;
		}
		// 정확한 원인은 모르나 내쪽에서 소켓 닫았을 확률 높은 것 같음
		else if (error == WSAECONNABORTED)
		{
			ReleaseSession(session);

			return;
		}
		else
		{
			LOG(L"WSARecv() error: %d", error);
			__debugbreak();
			ReleaseSession(session);

			return;
		}
	}
	//else
	//{
	//	printf("recv() Fast IO\n");
	//}
}


void NetServer::CompleteRecv(Session* session, int numOfBytes)
{
	session->recvQueue_.MoveRear(numOfBytes);

	if (session->recvQueue_.IsFull())
	{
		LOG_INFO(L"recvQueue full");

		ReleaseSession(session);

		return;
	}

	while (1)
	{
		int useSize = session->recvQueue_.UseSize();

		///헤더 변경시 수정//////////////////////////////////////////////////////////////////////////////////////
		NET_HEADER header;

		if (useSize < sizeof(header))
			break;

		session->recvQueue_.Peek((char*)&header, sizeof(header));

		if (header.code_ != NET_CODE)
		{
			//LOG_INFO(L"recv code error");

			ReleaseSession(session);

			return;
		}

		int bodySize = header.size_;

		if (bodySize < 0)
		{
			//LOG_INFO(L"body size minus error");

			ReleaseSession(session);

			return;
		}

		if (useSize < sizeof(header) + bodySize)
			break;

		//Packet* packet = new Packet();
		Packet* packet = packetPool_.Alloc();
		packet->Initialize(sizeof(header));

		// 헤더 + 바디 전부 디큐
		session->recvQueue_.Dequeue(packet->GetHeaderPtr(), sizeof(header) + bodySize);

		// 메세지 사이즈만큼만 무브
		packet->MoveWritePos(bodySize);

		/// 디코딩
		packet->Decoding();

		if (!packet->IsValidChecksum())
		{
			//LOG_INFO(L"recv checksum error");

			ReleaseSession(session);

			packet->Free();

			return;
		}

		recvMessageCount_++;

		OnRecv(session->sessionId_, packet);

		packet->Free();
	}

	InterlockedIncrement(&session->ioCount_);

	RecvPost(session);

	ReleaseSession(session);
}


//void NetServer::RecvPost(Session* session)
//{
//	ZeroMemory(&session->recvOverlapped_.overlapped, sizeof(WSAOVERLAPPED));
//
//	WSABUF wsaBuf[2];
//
//	wsaBuf[0].buf = session->recvPacket_->GetRearBufferPtr();
//	wsaBuf[0].len = session->recvPacket_->FreeSize();
//
//	DWORD flags = 0;
//
//	int retval = WSARecv(session->socket_, wsaBuf, 1, nullptr, &flags, (WSAOVERLAPPED*)&session->recvOverlapped_, NULL);
//
//	if (retval == SOCKET_ERROR)
//	{
//		int error = WSAGetLastError();
//
//		if (error == ERROR_IO_PENDING)
//		{
//			//printf("recv() Direct IO\n");
//		}
//		else if (error == WSAECONNRESET || error == WSAECONNABORTED)
//		{
//			ReleaseSession(session);
//
//			return;
//		}
//		else
//		{
//			LOG(L"WSARecv() error: %d", error);
//
//			__debugbreak();
//
//			ReleaseSession(session);
//
//			return;
//		}
//	}
//	else
//	{
//		//printf("recv() Fast IO\n");
//	}
//}
//
//void NetServer::CompleteRecv(Session* session, int numOfBytes)
//{
//	//PRO(L"CompleteRecv");
//
//	/// 수정 필요 (지금 패킷 수신중이라 계속 쪼개져서 와서 이걸론 확인 불가)
//	if (numOfBytes >= PACKET_CAPACITY)
//	{
//		LOG_INFO(L"recvQueue full");
//
//		ReleaseSession(session);
//
//		return;
//	}
//
//	int useSize = session->recvPacket_->UseSize();
//
//	int processedBytes = 0;
//	int totalBytes = numOfBytes + useSize;
//	int remainBytes = totalBytes;
//
//	session->recvPacket_->MoveWritePos(-useSize);
//
//	bool completeFirstLoop = false;
//
//	while (1)
//	{
//		if (remainBytes < sizeof(NET_HEADER))
//		{
//			break;
//		}
//
//		/// 코드 확인
//		unsigned char code = reinterpret_cast<NET_HEADER*>(session->recvPacket_->GetHeaderPtr() + processedBytes)->code_;
//
//		if (code != NET_CODE)
//		{
//			LOG_INFO(L"recv code error");
//
//			ReleaseSession(session);
//
//			return;
//		}
//
//		int bodySize = reinterpret_cast<NET_HEADER*>(session->recvPacket_->GetHeaderPtr() + processedBytes)->size_;
//
//		if (bodySize < 0)
//		{
//			LOG_INFO(L"recv size error");
//		
//			ReleaseSession(session);
//
//			return;
//		}
//
//		if (remainBytes < sizeof(NET_HEADER) + bodySize)
//		{
//			break;
//		}
//
//		if (!completeFirstLoop)
//		{
//			session->recvPacket_->MoveWritePos(sizeof(NET_HEADER) + bodySize);
//			session->recvPacket_->MoveReadPos(sizeof(NET_HEADER));
//
//			/// 디코딩
//			session->recvPacket_->Decoding();
//
//			if (!session->recvPacket_->IsValidChecksum())
//			{
//				LOG_INFO(L"recv checksum error");
//
//				ReleaseSession(session);
//
//				return;
//			}
//
//			processedBytes += (sizeof(NET_HEADER) + bodySize);
//			remainBytes = totalBytes - processedBytes;
//
//			++recvMessageCount_;
//
//			OnRecv(session->sessionId_, session->recvPacket_);
//
//			completeFirstLoop = true;
//		}
//		else
//		{
//			Packet* packet = packetPool_.Alloc();
//			packet->Clear(sizeof(NET_HEADER));
//
//			packet->Write(session->recvPacket_->GetHeaderPtr() + processedBytes, sizeof(NET_HEADER) + bodySize);
//			packet->MoveReadPos(sizeof(NET_HEADER));
//
//			/// 디코딩
//			packet->Decoding();
//
//			if (!packet->IsValidChecksum())
//			{
//				LOG_INFO(L"recv checksum error");
//
//				ReleaseSession(session);
//
//				packet->Free();
//
//				return;
//			}
//
//			processedBytes += (sizeof(NET_HEADER) + bodySize);
//			remainBytes = totalBytes - processedBytes;
//
//			++recvMessageCount_;
//
//			OnRecv(session->sessionId_, packet);
//
//			packet->Free();
//		}
//	}
//
//	Packet* newPacket = packetPool_.Alloc();
//	newPacket->Clear(sizeof(NET_HEADER));
//
//	if (remainBytes > 0)
//	{
//		newPacket->Write(session->recvPacket_->GetHeaderPtr() + processedBytes, remainBytes);
//	}
//
//	session->recvPacket_->Free();
//	session->recvPacket_ = newPacket;
//
//	// WSARecv() 이전에 호출
//	InterlockedIncrement(&session->ioCount_);
//
//	RecvPost(session);
//
//	ReleaseSession(session);
//}


#define MAX_WSABUF 100

void NetServer::SendPost(Session* session)
{
	if (InterlockedExchange(&session->sendPending_, 1) == 1)
		return;

	ZeroMemory(&session->sendOverlapped_.overlapped_, sizeof(WSAOVERLAPPED));

	//int count = min(session->sendQueue_.UseSize(), MAX_WSABUF);
	int count = min(session->sendQueue_.Size(), MAX_WSABUF);

	// WSASend() bufcount 인자 0이면 10022 에러 발생함. 예외처리
	if (count <= 0)
	{
		InterlockedExchange(&session->sendPending_, 0);

		return;
	}

	WSABUF wsaBuf[MAX_WSABUF];

	for (int i = 0; i < count; i++)
	{
		Packet* packet;

		session->sendQueue_.Dequeue(packet);

		wsaBuf[i].buf = packet->GetHeaderPtr();
		wsaBuf[i].len = packet->GetHeaderSize() + packet->UseSize();

		session->sendPostQueue_.Enqueue(packet);
	}


	InterlockedIncrement(&session->ioCount_);

	// wsaBuf count가 0이면 10022 에러 발생함
	int retval = WSASend(session->socket_, wsaBuf, count, nullptr, 0, (WSAOVERLAPPED*)&(session->sendOverlapped_), NULL);

	if (retval == SOCKET_ERROR)
	{
		int error = WSAGetLastError();

		if (error == ERROR_IO_PENDING)
		{
			//printf("recv() Direct IO\n");
		}
		else if (error == WSAECONNRESET || error == WSAESHUTDOWN || error == WSAECONNABORTED)
		{
			ReleaseSession(session);

			return;
		}
		// 10038 내가 연결을 끊었을때 (closesocket)
		// 세션 정리때 소켓을 닫는 구조라 이 에러가 정상적인 상황에선 불가능
		else if (error == WSAENOTSOCK)
		{
			__debugbreak();
			ReleaseSession(session);

			return;
		}
		else
		{
			LOG(L"WSARecv() error: %d", error);
			__debugbreak();
			ReleaseSession(session);

			return;
		}
	}
	//else
	//{
	//	printf("send Fast IO\n");
	//}
}

void NetServer::CompleteSend(Session* session, int numOfBytes)
{
	while (1)
	{
		Packet* packet;

		bool ret = session->sendPostQueue_.Dequeue(packet);

		if (ret == false)
			break;

		//delete p;
		packet->Free();
	}

	InterlockedExchange(&session->sendPending_, 0);

	//int s = session->sendQueue_.UseSize();
	int size = session->sendQueue_.Size();

	if (size > 0)
		SendPost(session);

	ReleaseSession(session);
}


void NetServer::ReleaseSession(Session* session)
{
	if (InterlockedDecrement(&session->ioCount_) == 0)
	{
		if (InterlockedCompareExchange(&session->ioCount_, RELEASE_MASK, 0) != 0)
			return;

		OnRelease(session->sessionId_);

		InterlockedDecrement(&sessionCount_);

		/// accept 스레드 사용 시
		closesocket(session->socket_);

		/// 추가
		session->Clear();


		int index = GetSessionIndex(session->sessionId_);
		sessionIndexPool_.Push(index);


		/// 소켓 연결 종료 (세션 사용 끝)
		//ZeroMemory(&session->disconnectOverlapped_.overlapped_, sizeof(WSAOVERLAPPED));

		//BOOL ret = lpfnDisconnectex_(session->socket_, (OVERLAPPED*)&session->disconnectOverlapped_, TF_REUSE_SOCKET, 0);

		//if (ret == FALSE)
		//{
		//	int error = WSAGetLastError();

		//	if (error == ERROR_IO_PENDING)
		//	{
		//		return;
		//	}
		//	else
		//	{
		//		wprintf(L"DisconnectEx failed with error: %d\n", error);
		//		__debugbreak();
		//	}
		//}
	}
}


uint64_t NetServer::AllocSessionId(Session* session)
{
	uint64_t index = session->sessionId_ >> SESSION_INDEX_SHIFT;

	uint64_t id = ++idSeed_;

	return id | (index << SESSION_INDEX_SHIFT);
}

uint64_t NetServer::GetSessionId(uint64_t sessionId)
{
	return sessionId & SESSION_ID_MASK;
}

int NetServer::GetSessionIndex(uint64_t sessionId)
{
	return static_cast<int>((sessionId & SESSION_INDEX_MASK) >> SESSION_INDEX_SHIFT);
}
