#pragma once
#include <vector>
#include <string>

#include "Player.h"
#include "OmokGame.h"
#include "Utils/Packet.h"


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

inline Packet& operator<<(Packet& packet, Player& player)
{
	packet << player.playerId_;
	packet << player.playerName_;
	packet << player.win_;
	packet << player.lose_;
	packet << player.rating_;
	packet << player.money_;
	packet << player.state_;

	return packet;
}

inline Packet& operator>>(Packet& packet, Player& player)
{
	packet >> player.playerId_;
	packet >> player.playerName_;
	packet >> player.win_;
	packet >> player.lose_;
	packet >> player.rating_;
	packet >> player.money_;
	packet >> player.state_;

	return packet;
}
