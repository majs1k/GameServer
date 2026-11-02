#include "NetServer.h"
#include "../Utils/Packet.h"
#include "../Utils/Logger.h"
#include "../Utils/Profiler.h"
#include <Windows.h>

//void NetServer::RecvPost(Session* session)
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
//void NetServer::CompleteRecv(Session* session, int numOfBytes)
//{
//	//PRO(L"CompleteRecv");
//
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
//		NET_HEADER header;
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
//		///헤더 변경시 수정//////////////////////////////////////////////////////////////////////////////////////
//		packet->Initialize(5, 1);
//
//		// 헤더 + 바디 전부 디큐
//		session->recvQueue_.Dequeue(packet->GetHeaderPtr(), sizeof(header) + bodySize);
//
//		// 메세지 사이즈만큼만 무브
//		packet->MoveWritePos(bodySize);
//
//		/// 디코딩
//		packet->Decoding();
//
//		if (!packet->IsValidChecksum())
//		{
//			LOG_INFO(L"recv checksum error");
//
//			Disconnect(session->sessionId_);
//
//			return;
//		}
//
//		InterlockedIncrement(&recvMessageCount_);
//
//		this->OnRecv(session->sessionId_, packet);
//
//		packet->Free();
//	}
//
//	// WSARecv() 이전에 호출
//	InterlockedIncrement(&session->ioCount_);
//
//	this->RecvPost(session);
//
//	this->ReleaseSession(session);
//}


void NetServer::RecvPost(Session* session)
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

void NetServer::CompleteRecv(Session* session, int numOfBytes)
{
	//PRO(L"CompleteRecv");

	if (numOfBytes >= PACKET_CAPACITY)
	{
		LOG_INFO(L"recvQueue full");

		Disconnect(session->sessionId_);

		return;
	}

	int useSize = session->recvPacket_->UseSize();

	int processedBytes = 0;
	int totalBytes = numOfBytes + useSize;
	int remainBytes = totalBytes;

	session->recvPacket_->MoveWritePos(-useSize);

	bool completeFirstLoop = false;

	while (1)
	{
		if (remainBytes < sizeof(NET_HEADER))
		{
			break;
		}

		/// 코드 확인
		unsigned char code = reinterpret_cast<NET_HEADER*>(session->recvPacket_->GetHeaderPtr() + processedBytes)->code_;

		if (code != NET_CODE)
		{
			LOG_INFO(L"recv code error");

			Disconnect(session->sessionId_);

			return;
		}

		int bodySize = reinterpret_cast<NET_HEADER*>(session->recvPacket_->GetHeaderPtr() + processedBytes)->size_;

		if (bodySize < 0)
		{
			LOG_INFO(L"recv size error");

			Disconnect(session->sessionId_);

			return;
		}

		if (remainBytes < sizeof(NET_HEADER) + bodySize)
		{
			break;
		}

		if (!completeFirstLoop)
		{
			session->recvPacket_->MoveWritePos(sizeof(NET_HEADER) + bodySize);
			session->recvPacket_->MoveReadPos(sizeof(NET_HEADER));

			/// 디코딩
			session->recvPacket_->Decoding();

			if (!session->recvPacket_->IsValidChecksum())
			{
				LOG_INFO(L"recv checksum error");

				Disconnect(session->sessionId_);

				return;
			}

			processedBytes += (sizeof(NET_HEADER) + bodySize);
			remainBytes = totalBytes - processedBytes;

			InterlockedIncrement(&recvMessageCount_);

			OnRecv(session->sessionId_, session->recvPacket_);

			completeFirstLoop = true;
		}
		else
		{
			Packet* packet = packetPool_.Alloc();
			packet->InitRecvPacket(sizeof(NET_HEADER));

			packet->Write(session->recvPacket_->GetHeaderPtr() + processedBytes, sizeof(NET_HEADER) + bodySize);

			packet->MoveWritePos(sizeof(NET_HEADER) + bodySize);
			packet->MoveReadPos(sizeof(NET_HEADER));

			/// 디코딩
			packet->Decoding();

			if (!packet->IsValidChecksum())
			{
				LOG_INFO(L"recv checksum error");

				packet->Free();

				Disconnect(session->sessionId_);

				return;
			}

			processedBytes += (sizeof(NET_HEADER) + bodySize);
			remainBytes = totalBytes - processedBytes;

			InterlockedIncrement(&recvMessageCount_);

			OnRecv(session->sessionId_, packet);

			packet->Free();
		}
	}

	Packet* newPacket = packetPool_.Alloc();
	newPacket->InitRecvPacket(sizeof(NET_HEADER));

	if (remainBytes > 0)
	{
		newPacket->Write(session->recvPacket_->GetHeaderPtr() + processedBytes, remainBytes);
	}

	session->recvPacket_->Free();
	session->recvPacket_ = newPacket;

	// WSARecv() 이전에 호출
	InterlockedIncrement(&session->ioCount_);

	RecvPost(session);

	ReleaseSession(session);
}




#define MAX_WSABUF 100

void NetServer::SendPost(Session* session)
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

void NetServer::CompleteSend(Session* session, int numOfBytes)
{
	while (1)
	{
		Packet* p;

		bool ret = session->sendPostQueue_.Dequeue(p);

		if (ret == false)
			break;

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

void NetServer::ReleaseSession(Session* session)
{
	if (InterlockedDecrement(&session->ioCount_) == 0)
	{
		if (InterlockedCompareExchange(&session->ioCount_, 0x80000000, 0) != 0)
			return;

		closesocket(session->socket_);

		int index = GetSessionIndex(session->sessionId_);

		sessionIndex_.Push(index);

		this->OnRelease(session->sessionId_);

		//LOG_INFO(L"[NETWORK] session delete");

		InterlockedDecrement(&sessionCount_);
	}
}

uint64_t NetServer::AllocSession()
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

uint64_t NetServer::GetSessionId(uint64_t sessionId)
{
	return sessionId & 0x0000ffffffffffff;
}

int NetServer::GetSessionIndex(uint64_t sessionId)
{
	return static_cast<int>((sessionId >> 48) & 0xffff);
}
