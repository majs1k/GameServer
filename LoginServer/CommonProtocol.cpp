#include "CommonProtocol.h"
#include "Utils/CommonUtility.h"

Email::Email(const std::string& email)
{
	std::wstring ws = StringToWString(email);

	wcscpy_s(email_, 20, ws.c_str());
}

Nickname::Nickname(const std::string& nickname)
{
	std::wstring ws = StringToWString(nickname);

	wcscpy_s(nickname_, 20, ws.c_str());
}

Chat::Chat(const wchar_t* chat)
{
	wcscpy_s(chat_, 100, chat);
	len_ = static_cast<short>(wcslen(chat_) * sizeof(wchar_t));
}

IpAddress::IpAddress(const std::string& ipAddress)
{
	std::wstring ws = StringToWString(ipAddress);

	wcscpy_s(ipAddress_, 16, ws.c_str());
}
