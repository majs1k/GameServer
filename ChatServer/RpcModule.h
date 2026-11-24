#pragma once
#include <vector>
#include <string>

#include "CommonProtocol.h"
#include "Rpc/RpcProxy.h"

// ChatServer 전용 프록시 전역객체
inline RpcProxy g_RpcProxy;


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

inline Packet& operator<<(Packet& packet, Email& email)
{
	packet.Write(reinterpret_cast<char*>(email.email_), sizeof(Email));
	return packet;
}

inline Packet& operator>>(Packet& packet, Email& email)
{
	packet.Read(reinterpret_cast<char*>(email.email_), sizeof(Email));
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

inline Packet& operator<<(Packet& packet, IpAddress& ipAddress)
{
	packet.Write(reinterpret_cast<char*>(ipAddress.ipAddress_), sizeof(Nickname));
	return packet;
}

inline Packet& operator>>(Packet& packet, IpAddress& ipAddress)
{
	packet.Read(reinterpret_cast<char*>(ipAddress.ipAddress_), sizeof(Nickname));
	return packet;
}
