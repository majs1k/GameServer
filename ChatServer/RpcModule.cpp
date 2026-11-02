// -------------------------------------------------
// 
// 직렬화 버퍼에 필요한 가변 인자들 전역 오버로딩
// RPC 클래스 컴파일
//
// -------------------------------------------------
#include <vector>
#include <string>

#include "RpcModule.h"
#include "Utils/Packet.h"

RpcProxy g_RpcProxy;


inline Packet& operator<<(Packet& packet, std::string& str)
{
	packet << static_cast<short>(str.size());

	packet.Write(str.data(), static_cast<short>(str.size()));

	return packet;
}

inline Packet& operator>>(Packet& packet, std::string& str)
{
	short size = 0;

	packet >> size;
	str.resize(size);

	packet.Read(const_cast<char*>(str.data()), size);

	return packet;
}

template<typename T>
inline Packet& operator<<(Packet& packet, std::vector<T>& vec)
{
	packet << static_cast<short>(vec.size());

	for (auto& v : vec)
		packet << v;

	return packet;
}

template<typename T>
inline Packet& operator>>(Packet& packet, std::vector<T>& vec)
{
	short size = 0;

	packet >> size;

	T data;

	for (int i = 0; i < size; i++)
	{
		packet >> data;
		vec.push_back(data);
	}

	return packet;
}

inline Packet& operator<<(Packet& packet, LoginId& loginId)
{
	packet.Write(reinterpret_cast<char*>(loginId.loginId_), sizeof(LoginId));

	return packet;
}

inline Packet& operator>>(Packet& packet, LoginId& loginId)
{
	packet.Read(reinterpret_cast<char*>(loginId.loginId_), sizeof(LoginId));

	return packet;
}

inline Packet& operator<<(Packet& packet, Nickname& nickname)
{
	packet.Write(reinterpret_cast<char*>(nickname.nickname_), sizeof(Nickname));

	return packet;
}

inline Packet& operator>>(Packet& packet, Nickname& nickname)
{
	packet.Read(reinterpret_cast<char*>(nickname.nickname_), sizeof(Nickname));

	return packet;
}

inline Packet& operator<<(Packet& packet, SessionKey& sessionKey)
{
	packet.Write(reinterpret_cast<char*>(sessionKey.sessionKey_), sizeof(SessionKey));

	return packet;
}

inline Packet& operator>>(Packet& packet, SessionKey& sessionKey)
{
	packet.Read(reinterpret_cast<char*>(sessionKey.sessionKey_), sizeof(SessionKey));

	return packet;
}

inline Packet& operator<<(Packet& packet, Chat& chat)
{
	packet << chat.len_;
	packet.Write(reinterpret_cast<char*>(chat.chat_), chat.len_);

	return packet;
}

inline Packet& operator>>(Packet& packet, Chat& chat)
{
	packet >> chat.len_;
	packet.Read(reinterpret_cast<char*>(chat.chat_), chat.len_);

	return packet;
}

#include "RPC/RpcProxy.cpp"
#include "RPC/RpcStub.cpp"
