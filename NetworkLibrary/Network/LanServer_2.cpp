#include "LanServer.h"
#include "../Utils/Packet.h"
#include "../Utils/Logger.h"
#include "../Utils/Profiler.h"
#include <Windows.h>

//void LanServer::RecvPost(Session* session)
//{
//	ZeroMemory(&session->recvOverlapped_.overlapped, sizeof(WSAOVERLAPPED));
//
//	WSABUF wsaBuf[2];
//
//	wsaBuf[0].buf = session->recvQueue_.GetRearBufferPtr();
//	wsaBuf[0].len = session->recvQueue_.DirectEnqueueSize();
//
//	DWORD flags = 0;
//	int retval;
//
//	if (session->recvQueue_.FreeSize() == session->recvQueue_.DirectEnqueueSize())
//	{
//		// buf 1개
//		retval = WSARecv(session->socket_, wsaBuf, 1, nullptr, &flags, (WSAOVERLAPPED*)&session->recvOverlapped_, NULL);
//	}
//	else
//	{
//		// buf 2개
//		wsaBuf[1].buf = session->recvQueue_.GetBufferPtr();
//		wsaBuf[1].len = session->recvQueue_.FreeSize() - session->recvQueue_.DirectEnqueueSize();
//
//		retval = WSARecv(session->socket_, wsaBuf, 2, nullptr, &flags, (WSAOVERLAPPED*)&session->recvOverlapped_, NULL);
//	}
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
//			this->ReleaseSession(session);
//
//			return;
//		}
//		else
//		{
//			LOG(L"WSARecv() error: %d, iocount: %d", error, session->ioCount_);
//			//__debugbreak();
//			this->ReleaseSession(session);
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
//
//void LanServer::CompleteRecv(Session* session, int numOfBytes)
//{
//	session->recvQueue_.MoveRear(numOfBytes);
//
//	if (session->recvQueue_.IsFull())
//	{
//		LOG_INFO(L"recvQueue full");
//
//		Disconnect(session->sessionId_);
//
//		return;
//	}
//
//	while (1)
//	{
//		int useSize = session->recvQueue_.UseSize();
//
//		///헤더 변경시 수정//////////////////////////////////////////////////////////////////////////////////////
//		LAN_HEADER header;
//
//		if (useSize < sizeof(header))
//			break;
//
//		session->recvQueue_.Peek((char*)&header, sizeof(header));
//
//		int bodySize = header.size_;
//
//		if (bodySize < 0)
//			break;
//
//		if (useSize < sizeof(header) + bodySize)
//			break;
//
//		//Packet* packet = new Packet();
//		Packet* packet = packetPool_.Alloc();
//		packet->Initialize(2, 1);
//
//		// 헤더 + 바디 전부 디큐
//		session->recvQueue_.Dequeue(packet->GetHeaderPtr(), sizeof(header) + bodySize);
//
//		// 메세지 사이즈만큼만 무브
//		packet->MoveWritePos(bodySize);
//
//		InterlockedIncrement(&recvMessageCount_);
//
//		this->OnRecv(session->sessionId_, packet);
//	}
//
//	// WSARecv() 이전에 호출
//	InterlockedIncrement(&session->ioCount_);
//
//	this->RecvPost(session);
//
//	this->ReleaseSession(session);
//}


void LanServer::RecvPost(Session* session)
{
	ZeroMemory(&session->recvOverlapped_.overlapped, sizeof(WSAOVERLAPPED));

	WSABUF wsaBuf[2];

	wsaBuf[0].buf = session->recvPacket_->GetRearBufferPtr();
	wsaBuf[0].len = session->recvPacket_->FreeSize();

	DWORD flags = 0;

	int retval = WSARecv(session->socket_, wsaBuf, 1, nullptr, &flags, (WSAOVERLAPPED*)&session->recvOverlapped_, NULL);

	if (retval == SOCKET_ERROR)
	{
		int error = WSAGetLastError();

		if (error == ERROR_IO_PENDING)
		{
			//printf("recv() Direct IO\n");
		}
		else if (error == WSAECONNRESET || error == WSAECONNABORTED)
		{
			this->ReleaseSession(session);

			return;
		}
		else
		{
			LOG(L"WSARecv() error: %d, iocount: %d", error, session->ioCount_);
			//__debugbreak();
			this->ReleaseSession(session);

			return;
		}
	}
	else
	{
		//printf("recv() Fast IO\n");
	}
}

void LanServer::CompleteRecv(Session* session, int numOfBytes)
{
	if (numOfBytes >= PACKET_CAPACITY)
	{
		LOG_INFO(L"recvQueue full");

		Disconnect(session->sessionId_);

		return;
	}

	session->recvPacket_->AddRef();

	int recvBytes =0;
	int remainBytes = numOfBytes;

	Packet* rawPacket = session->recvPacket_;
	Packet* newPacket = rawPacket;

	while (1)
	{
		if (remainBytes < sizeof(LAN_HEADER))
		{
			session->recvPacket_->MoveWritePos(numOfBytes);

			break;
		}
		
		int bodySize = reinterpret_cast<LAN_HEADER*>(rawPacket->GetHeaderPtr() + recvBytes)->size_;

		if (bodySize < 0)
		{
			if(newPacket != rawPacket)
				newPacket->Free();

			LOG_INFO(L"recv size error");

			ReleaseSession(session);

			return;
		}

		if (remainBytes < sizeof(LAN_HEADER) + bodySize)
		{
			session->recvPacket_->MoveWritePos(numOfBytes);

			break;
		}

		if (newPacket != rawPacket)
		{
			newPacket->Write(rawPacket->GetHeaderPtr() + recvBytes, sizeof(LAN_HEADER) + bodySize);
		}

		session->recvPacket_->MoveWritePos(sizeof(LAN_HEADER) + bodySize);


		recvBytes += (sizeof(LAN_HEADER) + bodySize);
		remainBytes = numOfBytes - recvBytes;


		InterlockedIncrement(&recvMessageCount_);

		OnRecv(session->sessionId_, newPacket);


		newPacket = packetPool_.Alloc();
		newPacket->InitRecvPacket(sizeof(LAN_HEADER));
	}

	session->recvPacket_->Free();
	session->recvPacket_ = newPacket;


	// WSARecv() 이전에 호출
	InterlockedIncrement(&session->ioCount_);

	RecvPost(session);

	ReleaseSession(session);
}


#define MAX_WSABUF 100

void LanServer::SendPost(Session* session)
{
	if (InterlockedExchange(&session->sendPending_, 1) == 1)
		return;

	ZeroMemory(&session->sendOverlapped_.overlapped, sizeof(WSAOVERLAPPED));

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
			//printf("send Direct IO\n");
		}
		else if (error == WSAECONNRESET || error == WSAECONNABORTED || error == WSAENOTSOCK)
		{
			this->ReleaseSession(session);

			return;
		}
		else
		{
			LOG(L"WSASend() error: %d", error);
			__debugbreak();

			this->ReleaseSession(session);
			return;
		}
	}
	else
	{
		//printf("send Fast IO\n");
	}
}

void LanServer::CompleteSend(Session* session, int numOfBytes)
{
	while (!session->sendPostQueue_.IsEmpty())
	{
		Packet* p = session->sendPostQueue_.Dequeue();

		//delete p;
		p->Free();
	}

	InterlockedExchange(&session->sendPending_, 0);

	//int s = session->sendQueue_.UseSize();
	int s = session->sendQueue_.Size();

	if (s > 0)
		this->SendPost(session);

	this->ReleaseSession(session);
}

void LanServer::ReleaseSession(Session* session)
{
	if (InterlockedDecrement(&session->ioCount_) == 0)
	{
		if (InterlockedCompareExchange(&session->ioCount_, 0x80000000, 0) != 0)
			return;

		closesocket(session->socket_);
		packetPool_.Free(session->recvPacket_);

		int index = GetSessionIndex(session->sessionId_);

		sessionIndex_.Push(index);

		this->OnRelease(session->sessionId_);

		//LOG_INFO(L"[NETWORK] session delete");

		InterlockedDecrement(&sessionCount_);
	}
}

uint64_t LanServer::AllocSession()
{
	int index;

	//sessionMapLock_.lock();
	//index = sessionIndex_.top();
	//sessionIndex_.pop();
	//sessionMapLock_.unlock();

	sessionIndex_.Pop(index);

	// idSeed_는 accept 스레드에서만 변경 가능
	return ++idSeed_ | (static_cast<uint64_t>(index) << 48);
}

uint64_t LanServer::GetSessionId(uint64_t sessionId)
{
	return sessionId & 0x0000ffffffffffff;
}

int LanServer::GetSessionIndex(uint64_t sessionId)
{
	return static_cast<int>((sessionId >> 48) & 0xffff);
}
