#pragma once
//#include <Windows.h>


// 섹터 50 x 50

enum en_PACKET_TYPE
{
	////////////////////////////////////////////////////////
	//
	//	Client & Server Protocol
	//
	////////////////////////////////////////////////////////

	//------------------------------------------------------
	// Chatting Server
	//------------------------------------------------------
	en_PACKET_CS_CHAT_SERVER = 0,

	//------------------------------------------------------------
	// 채팅서버 로그인 요청
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]				// null 포함
	//		WCHAR	Nickname[20]		// null 포함
	//		char	SessionKey[64];		// 인증토큰
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_LOGIN,

	//------------------------------------------------------------
	// 채팅서버 로그인 응답
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status				// 0:실패	1:성공
	//		INT64	AccountNo
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_RES_LOGIN,

	//------------------------------------------------------------
	// 채팅서버 섹터 이동 요청
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_SECTOR_MOVE,

	//------------------------------------------------------------
	// 채팅서버 섹터 이동 결과
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_RES_SECTOR_MOVE,

	//------------------------------------------------------------
	// 채팅서버 채팅보내기 요청
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null 미포함
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_MESSAGE,

	//------------------------------------------------------------
	// 채팅서버 채팅보내기 응답  (다른 클라가 보낸 채팅도 이걸로 받음)
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]						// null 포함
	//		WCHAR	Nickname[20]				// null 포함
	//		
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null 미포함
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_RES_MESSAGE,

	//------------------------------------------------------------
	// 하트비트
	//
	//	{
	//		WORD		Type
	//	}
	//
	//
	// 클라이언트는 이를 30초마다 보내줌.
	// 서버는 40초 이상동안 메시지 수신이 없는 클라이언트를 강제로 끊어줘야 함.
	//------------------------------------------------------------	
	en_PACKET_CS_CHAT_REQ_HEARTBEAT,



};

//#pragma pack(push, 1)
//
//struct REQ_LOGIN
//{
//	WORD	Type;
//
//	INT64	AccountNo;
//	WCHAR	ID[20];				// null 포함
//	WCHAR	Nickname[20];		// null 포함
//	char	SessionKey[64];		// 인증토큰
//};
//
//struct RES_LOGIN
//{
//	WORD	Type;
//
//	BYTE	Status;				// 0:실패	1:성공
//	INT64	AccountNo;
//};
//
//struct REQ_SECTOR_MOVE
//{
//	WORD	Type;
//
//	INT64	AccountNo;
//	WORD	SectorX;
//	WORD	SectorY;
//};
//
//struct RES_SECTOR_MOVE
//{
//	WORD	Type;
//
//	INT64	AccountNo;
//	WORD	SectorX;
//	WORD	SectorY;
//};
//
//struct REQ_MESSAGE
//{
//	WORD	Type;
//
//	INT64	AccountNo;
//	WCHAR	ID[20];				// null 포함
//	WCHAR	Nickname[20];		// null 포함
//
//	WORD	MessageLen;			// 총 바이트수
//	WCHAR* Message;				// null 미포함
//};
//
//struct RES_MESSAGE
//{
//	WORD	Type;
//
//	INT64	AccountNo;
//	WCHAR	ID[20];				// null 포함
//	WCHAR	Nickname[20];		// null 포함
//
//	WORD	MessageLen;
//	WCHAR* Message;				// null 미포함
//};
//
//struct REQ_HEARTBEAT
//{
//	WORD		Type;
//};
//
//#pragma pack(pop)


struct LoginId
{
	wchar_t loginId_[20];
};

struct Nickname
{
	wchar_t nickname_[20];
};

struct SessionKey
{
	char sessionKey_[64];

	bool operator==(const SessionKey& other) const
	{
		return strcmp(sessionKey_, other.sessionKey_) == 0;
	}

	bool operator!=(const SessionKey& other) const
	{
		return !(*this == other);
	}
};

struct Chat
{
	short len_;
	wchar_t chat_[100];

	Chat() = default;

	Chat(const wchar_t* chat)
	{
		wcscpy_s(chat_, 100, chat);
		len_ = wcslen(chat_) * sizeof(wchar_t);
	}
};

enum ELOGIN
{
	FAIL = 0,
	SUCCESS = 1,
};


struct Pkt_SectorMove
{
	int64_t playerId_;
	unsigned short sectorX_;
	unsigned short sectorY_;
};

struct Pkt_Chat
{
	int64_t playerId_;
	LoginId loginId_;
	Nickname nickname_;
	Chat chat_;
	unsigned short sectorX_;
	unsigned short sectorY_;
};
