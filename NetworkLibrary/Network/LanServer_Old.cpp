
bool LanServer::Disconnect(uint64_t sessionId)
{
	sessionMapLock_.lock();

	auto it = sessionMap_.find(sessionId);

	if (it == sessionMap_.end())
	{
		sessionMapLock_.unlock();

		return false;
	}

	Session* session = (*it).second;

	sessionMapLock_.unlock();

	closesocket(session->socket_);

	return true;
}

bool LanServer::SendPacket(uint64_t sessionId, Packet* packet)
{
	sessionMapLock_.lock();

	auto it = sessionMap_.find(sessionId);

	if (it == sessionMap_.end())
	{
		sessionMapLock_.unlock();

		return false;
	}

	Session* session = (*it).second;

	session->sessionLock_.lock();

	sessionMapLock_.unlock();


	packet->SetTestHeader();


	//session->sendQueue_.Enqueue(packet);
	session->sendQueue_.Enqueue(packet->GetHeaderPtr(), packet->GetHeaderSize() + packet->UseSize());

	/// 수정
	delete packet;

	InterlockedIncrement(&sendMessageCount_);

	this->SendPost(session);

	session->sessionLock_.unlock();

	return true;
}

unsigned int __stdcall LanServer::AcceptThread(void* param)
{
	LanServer* server = (LanServer*)param;

	while (1)
	{
		SOCKADDR_IN clientAddr;
		int addrLen;
		addrLen = sizeof(clientAddr);

		SOCKET clientSock = accept(server->listenSocket_, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSock == INVALID_SOCKET)
		{
			int error = WSAGetLastError();

			wprintf(L"accept() error: %d", error);

			continue;
		}

		InterlockedIncrement(&server->acceptCount_);

		CHAR str[16];
		InetNtopA(AF_INET, &clientAddr.sin_addr, str, 16);
		std::string ip = str;
		int port = ntohs(clientAddr.sin_port);

		// 허용되지 않은 ip와 port라면 바로 연결을 끊음
		if (!server->OnConnectionRequest(ip, port))
		{
			closesocket(clientSock);
			continue;
		}

		Session* session = new Session();

		// idSeed_는 이 스레드에서만 변경 가능
		session->Initialize(clientSock, ip, port, ++(server->idSeed_));

		server->sessionMapLock_.lock();

		server->sessionMap_.insert({ session->sessionId_, session });

		server->sessionMapLock_.unlock();

		InterlockedIncrement(&(server->sessionCount_));

		LOG_INFO(L"[NETWORK] session create ip=%S port=%d count=%d", ip.c_str(), port, server->sessionCount_);

		CreateIoCompletionPort((HANDLE)clientSock, server->hIOCP_, (ULONG_PTR)session, 0);

		server->RecvPost(session);

		// 위치 주의
		server->OnAccept(session->sessionId_);
	}

	return 0;
}

void LanServer::RecvPost(Session* session)
{
	ZeroMemory(&session->recvOverlapped_.overlapped, sizeof(WSAOVERLAPPED));

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
	session->recvQueue_.MoveRear(numOfBytes);

	while (1)
	{
		int useSize = session->recvQueue_.UseSize();

		if (session->recvQueue_.IsFull())
		{
			LOG_INFO(L"recvQueue full");

			//연결 종료 로직

			return;
		}

		///헤더 변경시 수정//////////////////////////////////////////////////////////////////////////////////////
		if (useSize < sizeof(MY_HEADER))
			break;

		MY_HEADER header;

		session->recvQueue_.Peek((char*)&header, sizeof(MY_HEADER));

		int messageSize = header.size_;

		if (messageSize < 0)
			break;

		if (useSize < sizeof(MY_HEADER) + messageSize)
			break;

		//session->recvQueue_.MoveFront(sizeof(TEST_HEADER));

		Packet* packet = new Packet();
		packet->Initialize();

		// 헤더 + 바디 전부 디큐
		session->recvQueue_.Dequeue(packet->GetBufferPtr(), messageSize + sizeof(MY_HEADER));

		// 메세지 사이즈만큼만 무브
		packet->MoveWritePos(messageSize);

		InterlockedIncrement(&recvMessageCount_);

		this->OnRecv(session->sessionId_, packet);
	}

	// WSARecv() 이전에 호출
	InterlockedIncrement(&session->ioCount_);

	this->RecvPost(session);

	this->ReleaseSession(session);
}



void LanServer::SendPost(Session* session)
{
	if (InterlockedExchange(&session->sendPending_, 1) == 1)
	{
		return;
	}

	ZeroMemory(&session->sendOverlapped_.overlapped, sizeof(WSAOVERLAPPED));

	WSABUF wsaBuf[2];

	int retval;

	session->sessionLock_.lock();

	if (session->sendQueue_.UseSize() <= session->sendQueue_.DirectDequeueSize())
	{
		// buf 1개
		wsaBuf[0].buf = session->sendQueue_.GetFrontBufferPtr();
		wsaBuf[0].len = session->sendQueue_.UseSize();

		session->sessionLock_.unlock();

		InterlockedIncrement(&session->ioCount_);

		retval = WSASend(session->socket_, wsaBuf, 1, nullptr, 0, (WSAOVERLAPPED*)&(session->sendOverlapped_), NULL);
	}
	else
	{
		// buf 2개
		wsaBuf[0].buf = session->sendQueue_.GetFrontBufferPtr();
		wsaBuf[0].len = session->sendQueue_.DirectEnqueueSize();

		wsaBuf[1].buf = session->sendQueue_.GetBufferPtr();
		wsaBuf[1].len = session->sendQueue_.UseSize() - session->sendQueue_.DirectEnqueueSize();

		session->sessionLock_.unlock();

		InterlockedIncrement(&session->ioCount_);

		retval = WSASend(session->socket_, wsaBuf, 2, nullptr, 0, (WSAOVERLAPPED*)&(session->sendOverlapped_), NULL);
	}

	if (retval == SOCKET_ERROR)
	{
		int error = WSAGetLastError();

		if (error == ERROR_IO_PENDING)
		{
			//printf("send Direct IO\n");
		}
		else if (error == WSAECONNRESET || error == WSAECONNABORTED)
		{
			this->ReleaseSession(session);

			return;
		}
		else
		{
			LOG(L"WSASend() error: %d, iocount: %d", error, session->ioCount_);
			//__debugbreak();
			this->ReleaseSession(session);

			return;
		}
	}
}

void LanServer::CompleteSend(Session* session, int numOfBytes)
{
	session->sendQueue_.MoveFront(numOfBytes);

	InterlockedExchange(&session->sendPending_, 0);

	session->sessionLock_.lock();

	int s = session->sendQueue_.UseSize();

	session->sessionLock_.unlock();

	if (s > 0)
		this->SendPost(session);

	this->ReleaseSession(session);
}

void LanServer::ReleaseSession(Session* session)
{
	if (InterlockedDecrement(&session->ioCount_) == 0)
	{
		sessionMapLock_.lock();

		sessionMap_.erase(session->sessionId_);

		session->sessionLock_.lock();

		sessionMapLock_.unlock();

		session->sessionLock_.unlock();

		this->OnRelease(session->sessionId_);

		delete session;

		LOG_INFO(L"[NETWORK] session delete");

		InterlockedDecrement(&sessionCount_);
	}
}
