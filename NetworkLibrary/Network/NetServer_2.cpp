#include "NetServer.h"
#include "../Utils/Packet.h"
#include "../Utils/Logger.h"
#include "../Utils/Profiler.h"
#include <Windows.h>


void NetServer::RecvPost(Session* session)
{
	ZeroMemory(&session->recvOverlapped_.overlapped_, sizeof(WSAOVERLAPPED));

	WSABUF wsaBuf[2];
	DWORD flags = 0;
	int bufferCount = 1;

	wsaBuf[0].buf = session->recvQueue_.GetRearBufferPtr();
	wsaBuf[0].len = session->recvQueue_.DirectEnqueueSize();

	if (session->recvQueue_.FreeSize() != session->recvQueue_.DirectEnqueueSize())
	{
		wsaBuf[1].buf = session->recvQueue_.GetBufferPtr();
		wsaBuf[1].len = session->recvQueue_.FreeSize() - session->recvQueue_.DirectEnqueueSize();

		bufferCount = 2;
	}

	int retval = WSARecv(session->socket_, wsaBuf, bufferCount, nullptr, &flags, (WSAOVERLAPPED*)&session->recvOverlapped_, NULL);

	if (retval == SOCKET_ERROR)
	{
		int error = WSAGetLastError();

		if (error == ERROR_IO_PENDING)
			return;

		// 10053 shutdown 시 알수 없는 이유로  매우 적은 확률로 이 에러가 발생
		if (error == WSAECONNRESET || error == WSAESHUTDOWN || error == WSAECONNABORTED)
		{
			ReleaseSession(session);
			return;
		}

		// 10038 내가 연결을 끊었을때 (closesocket)
		// 세션 정리때 소켓을 닫는 구조라 이 에러가 정상적인 상황에선 불가능
		if (error == WSAENOTSOCK)
		{
			__debugbreak();
			ReleaseSession(session);
			return;
		}

		LOG("NETWORK", ELOG::ERROR_LEVEL, "WSARecv() error=%d / id=0x%016llx", error, session->sessionId_);
		__debugbreak();
		ReleaseSession(session);
	}
}

void NetServer::CompleteRecv(Session* session, int numOfBytes)
{
	session->recvQueue_.MoveRear(numOfBytes);

	if (session->recvQueue_.IsFull())
	{
		//LOG("NETWORK", ELOG::ERROR_LEVEL, "recvQ full / id=0x%016llx", session->sessionId_);
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

		/// 헤더 코드 확인
		if (header.code_ != NET_CODE)
		{
			//LOG("NETWORK", ELOG::ERROR_LEVEL, "recv code error / id=0x%016llx", session->sessionId_);
			ReleaseSession(session);
			return;
		}

		int bodySize = header.size_;

		if (bodySize < 0 || PACKET_MAX_SIZE < bodySize)
		{
			//LOG("NETWORK", ELOG::ERROR_LEVEL, "recv body size error / id=0x%016llx", session->sessionId_);
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

		// 메세지 사이즈만큼 무브
		packet->MoveWritePos(bodySize);

		/// 디코딩
		packet->Decoding();

		if (!packet->IsValidChecksum())
		{
			//LOG("NETWORK", ELOG::ERROR_LEVEL, "recv checksum error / id=0x%016llx", session->sessionId_);
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
//	ZeroMemory(&session->recvOverlapped_.overlapped_, sizeof(WSAOVERLAPPED));
//
//	WSABUF wsaBuf;
//	DWORD flags = 0;
//
//	wsaBuf.buf = session->recvPacket_->GetRearBufferPtr();
//	wsaBuf.len = session->recvPacket_->FreeSize();
//
//	int retval = WSARecv(session->socket_, &wsaBuf, 1, nullptr, &flags, (WSAOVERLAPPED*)&session->recvOverlapped_, NULL);
//
//	if (retval == SOCKET_ERROR)
//	{
//		int error = WSAGetLastError();
//
//		if (error == ERROR_IO_PENDING)
//			return;
//
//		if (error == WSAECONNRESET || error == WSAESHUTDOWN)
//		{
//			ReleaseSession(session);
//			return;
//		}
//
//		// 10038 내가 연결을 끊었을때 (closesocket)
//		// 세션 정리때 소켓을 닫는 구조라 이 에러가 정상적인 상황에선 불가능
//		if (error == WSAENOTSOCK)
//		{
//			__debugbreak();
//			ReleaseSession(session);
//			return;
//		}
//
//		// 정확한 원인은 모르나 내쪽에서 소켓 닫았을 확률 높은 것 같음
//		if (error == WSAECONNABORTED)
//		{
//			ReleaseSession(session);
//			return;
//		}
//
//		LOG("NETWORK", ELOG::ERROR_LEVEL, "WSARecv() error=%d / id=0x%016llx", error, session->sessionId_);
//		__debugbreak();
//		ReleaseSession(session);
//	}
//}
//
//void NetServer::CompleteRecv(Session* session, int numOfBytes)
//{
//	int useSize = session->recvPacket_->UseSize();
//	int processedBytes = 0;
//	int totalBytes = numOfBytes + useSize;
//	int remainBytes = totalBytes;
//	int headerSize = sizeof(NET_HEADER);
//	bool completeFirstLoop = false;
//
//	if (totalBytes >= PACKET_CAPACITY)
//	{
//		LOG("NETWORK", ELOG::ERROR_LEVEL, "recvQueue full");
//		ReleaseSession(session);
//		return;
//	}
//
//	session->recvPacket_->MoveWritePos(-useSize);
//
//	while (1)
//	{
//		if (remainBytes < headerSize)
//			break;
//
//		/// 헤더 코드 확인
//		unsigned char code = reinterpret_cast<NET_HEADER*>(session->recvPacket_->GetHeaderPtr() + processedBytes)->code_;
//
//		if (code != NET_CODE)
//		{
//			LOG("NETWORK", ELOG::ERROR_LEVEL, "recv code error / id=0x%016llx", session->sessionId_);
//			ReleaseSession(session);
//			return;
//		}
//
//		int bodySize = reinterpret_cast<NET_HEADER*>(session->recvPacket_->GetHeaderPtr() + processedBytes)->size_;
//
//		if (bodySize < 0)
//		{
//			LOG("NETWORK", ELOG::ERROR_LEVEL, "recv body size error / id=0x%016llx", session->sessionId_);
//			ReleaseSession(session);
//			return;
//		}
//
//		if (remainBytes - headerSize < bodySize)
//			break;
//
//		if (!completeFirstLoop)
//		{
//			session->recvPacket_->MoveWritePos(headerSize + bodySize);
//			session->recvPacket_->MoveReadPos(headerSize);
//
//			/// 디코딩
//			session->recvPacket_->Decoding();
//
//			if (!session->recvPacket_->IsValidChecksum())
//			{
//				LOG("NETWORK", ELOG::ERROR_LEVEL, "recv checksum error / id=0x%016llx", session->sessionId_);
//				ReleaseSession(session);
//				return;
//			}
//
//			processedBytes += (headerSize + bodySize);
//			remainBytes = totalBytes - processedBytes;
//
//			++recvMessageCount_;
//			OnRecv(session->sessionId_, session->recvPacket_);
//
//			completeFirstLoop = true;
//
//			// 테스트용
//			//break;
//		}
//		else
//		{
//			Packet* packet = packetPool_.Alloc();
//			packet->Clear(headerSize);
//
//			packet->Write(session->recvPacket_->GetHeaderPtr() + processedBytes, headerSize + bodySize);
//			packet->MoveReadPos(headerSize);
//
//			/// 디코딩
//			packet->Decoding();
//
//			if (!packet->IsValidChecksum())
//			{
//				LOG("NETWORK", ELOG::ERROR_LEVEL, "recv checksum error / id=0x%016llx", session->sessionId_);
//				ReleaseSession(session);
//
//				packet->Free();
//				return;
//			}
//
//			processedBytes += (headerSize + bodySize);
//			remainBytes = totalBytes - processedBytes;
//
//			++recvMessageCount_;
//			OnRecv(session->sessionId_, packet);
//
//			packet->Free();
//		}
//	}
//
//	/// TODO: 삭제
//	//while (1)
//	//{
//	//	if (remainBytes < headerSize)
//	//		break;
//
//	//	NET_HEADER* header = reinterpret_cast<NET_HEADER*>(session->recvPacket_->GetHeaderPtr() + processedBytes);
//
//	//	if (header->code_ != NET_CODE)
//	//	{
//	//		ReleaseSession(session);
//	//		return;
//	//	}
//
//	//	int bodySize = header->size_;
//
//	//	if (bodySize < 0)
//	//	{
//	//		ReleaseSession(session);
//	//		return;
//	//	}
//
//	//	if (remainBytes - headerSize < bodySize)
//	//		break;
//
//	//	int packetSize = headerSize + bodySize;
//
//
//	//	Packet* packet = packetPool_.Alloc();
//	//	packet->Clear(headerSize);
//
//	//	packet->Write(session->recvPacket_->GetHeaderPtr() + processedBytes, packetSize);
//	//	packet->MoveReadPos(headerSize);
//
//	//	packet->Decoding();
//
//	//	if (!packet->IsValidChecksum())
//	//	{
//	//		ReleaseSession(session);
//	//		packet->Free();
//	//		return;
//	//	}
//
//	//	processedBytes += packetSize;
//	//	remainBytes = totalBytes - processedBytes;
//
//	//	++recvMessageCount_;
//
//	//	OnRecv(session->sessionId_, packet);
//
//	//	packet->Free();
//	//}
//
//
//	Packet* newPacket = packetPool_.Alloc();
//	newPacket->Clear(headerSize);
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
//	RecvPost(session);
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

		session->sendPostQueue_.push(packet);
	}

	InterlockedIncrement(&session->ioCount_);
	int retval = WSASend(session->socket_, wsaBuf, count, nullptr, 0, (WSAOVERLAPPED*)&(session->sendOverlapped_), NULL);

	if (retval == SOCKET_ERROR)
	{
		int error = WSAGetLastError();

		if (error == ERROR_IO_PENDING)
			return;

		// 10022 shutdown 시 알수 없는 이유로  매우 적은 확률로 이 에러가 발생
		if (error == WSAECONNRESET  || error == WSAESHUTDOWN ||  error == WSAECONNABORTED || error == WSAEINVAL)
		{
			ReleaseSession(session);
			return;
		}

		// 10038 내가 연결을 끊었을때 (closesocket)
		// 세션 정리때 소켓을 닫는 구조라 이 에러가 정상적인 상황에선 불가능
		if (error == WSAENOTSOCK)
		{
			__debugbreak();
			ReleaseSession(session);
			return;
		}

		LOG("NETWORK", ELOG::ERROR_LEVEL, "WSASend() error=%d / id=0x%016llx", error, session->sessionId_);
		__debugbreak();
		ReleaseSession(session);
		return;
	}
}

void NetServer::CompleteSend(Session* session, int numOfBytes)
{
	while (1)
	{
		if (session->sendPostQueue_.empty())
			break;

		Packet* packet = session->sendPostQueue_.front();
		session->sendPostQueue_.pop();
		packet->Free();
	}

	InterlockedExchange(&session->sendPending_, 0);

	//int size = session->sendQueue_.UseSize();
	int size = session->sendQueue_.Size();

	if (size > 0)
	{
		SendPost(session);
	}

	ReleaseSession(session);
}

void NetServer::ReleaseSession(Session* session)
{
	if (InterlockedDecrement(&session->ioCount_) == 0)
	{
		if (InterlockedCompareExchange(&session->ioCount_, RELEASE_MASK, 0) != 0)
			return;

		/// 디버깅용
		//{
		//	std::lock_guard<std::mutex> lock(debuggingLock_);

		//	auto iter = debuggingMap_.find(session->sessionId_);

		//	if (iter == debuggingMap_.end() || iter->second == 0)
		//	{
		//		__debugbreak();
		//	}
		//}

		// 컨텐츠에 세션 종료를 알림
		OnRelease(session->sessionId_);
		InterlockedDecrement(&sessionCount_);

		/// accept 스레드 사용 시
		closesocket(session->socket_);
		LOG("NETWORK", ELOG::SYSTEM_LEVEL, "session release / id=0x%016llx / so=0x%016llx", session->sessionId_, session->socket_);

		session->Clear();
		int index = GetSessionIndex(session->sessionId_);
		sessionIndexPool_.Push(index);

		/// AcceptEx() 사용 시
		//ZeroMemory(&session->disconnectOverlapped_.overlapped_, sizeof(WSAOVERLAPPED));

		//BOOL ret = lpfnDisconnectex_(session->socket_, (OVERLAPPED*)&session->disconnectOverlapped_, TF_REUSE_SOCKET, 0);

		//if (ret == FALSE)
		//{
		//	int error = WSAGetLastError();

		//	if (error == ERROR_IO_PENDING)
		//		return;
		//	
		//	LOG("NETWORK", ELOG::ERROR_LEVEL, "DisconnectEx error=%d / id=0x%016llx", error, session->sessionId_);
		//	__debugbreak();
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
