//-------------------------------------------------------------------------
// 
// 로그
// 
// 유니코드 사용해야함 주의
// Log에서 문자열 출력시 무조건 %s로 사용할것 (유니코드라도)
// 
// 예외조건 항상 꼼꼼히 체크할것 why? 멀티스레드에서는 어떤 오류가 발생할지 모름
// 서버 다운되도 데이터 오염은 never -> 로그 & 메모리덤프로 디버깅
// 
// wofstream 파일 출력해도 파일 인코딩은 utf-8
// 
//-------------------------------------------------------------------------
#pragma once
#include <fstream>
#include "Singleton.h"
#include <Windows.h>

enum class LogLevel
{
	DEFAULT,
	INFO,
	ERR,
};

#define LOG(fmt, ...)		        Logger::Instance().Log(LogLevel::DEFAULT, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)          Logger::Instance().Log(LogLevel::INFO, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)         Logger::Instance().Log(LogLevel::ERR, fmt, ##__VA_ARGS__)

class Logger : public Singleton<Logger>
{
private:
	std::wofstream fout_;
	WCHAR fileName_[MAX_PATH];

public:
	Logger()
	{
		SYSTEMTIME st;
		GetLocalTime(&st);

		swprintf_s(fileName_, L"./Log/log_%04d%02d%02d_%02d%02d%02d.txt", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

		fout_.open(fileName_);

		if (!fout_.is_open())
		{
			printf("*** Logger fail ***\n");

			__debugbreak();
		}
	}

	~Logger()
	{
		fout_.close();
	}

	void Log(LogLevel level, const WCHAR* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);

		WCHAR userMessage[1024];
		vswprintf_s(userMessage, fmt, args);

		va_end(args);

		SYSTEMTIME st;
		GetLocalTime(&st);

		DWORD threadId = GetCurrentThreadId();

		const WCHAR* levelStr;

		switch (level)
		{
		case LogLevel::INFO:
			levelStr = L"INFO";
			break;

		case LogLevel::ERR:
			levelStr = L"ERROR";
			break;

		default:
			levelStr = L"DEFAULT";
			break;
		}

		WCHAR finalMessage[1400];

		swprintf_s(finalMessage, L"[%04d-%02d-%02d %02d:%02d:%02d.%03d] [T%u] [%s] %s\n",
			st.wYear, st.wMonth, st.wDay,
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
			threadId, levelStr, userMessage);

		//std::wcout << finalMessage;
		fout_ << finalMessage;
		fout_.flush();
	}
};
