#pragma once
#include <string>
#include <Windows.h>

inline std::wstring StringToWString(const std::string& str, UINT codePage = CP_UTF8)
{
	if (str.empty())
		return L"";

	int len = MultiByteToWideChar(codePage, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);

	if (len <= 0)
		return L"";

	std::wstring wstr(len, L'\0');

	MultiByteToWideChar(codePage, 0, str.data(), static_cast<int>(str.size()), &wstr[0], len);
	return wstr;
}

inline std::string WStringToString(const std::wstring& wstr, UINT codePage = CP_UTF8)
{
	if (wstr.empty())
		return "";

	int len = WideCharToMultiByte(codePage, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);

	if (len <= 0)
		return "";

	std::string str(len, '\0');

	WideCharToMultiByte(codePage, 0, wstr.data(), static_cast<int>(wstr.size()), str.data(), len, nullptr, nullptr);
	return str;
}
