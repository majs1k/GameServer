#pragma once
#include <fstream>
#include <mutex>
#include <cstdarg>
#include <cstdio>
#include <strsafe.h>
#include <string>
#include <unordered_map>
#include <Windows.h>

enum class ELOG
{
	DEBUG_LEVEL,
	ERROR_LEVEL,
	SYSTEM_LEVEL,
};

enum class ELOG_MODE
{
	NONE_MODE,
	DEBUG_MODE,
	ERROR_MODE,
};

class Logger
{
private:

	std::unordered_map<std::string, std::ofstream> logFiles_;
	char directory_[MAX_PATH] = {};
	std::mutex lock_;
	ELOG_MODE mode_ = ELOG_MODE::DEBUG_MODE;

private:

	bool OpenLogFile(const char* fileDirectory)
	{
		if (!CreateDirectoryA(directory_, nullptr))
		{
			if (GetLastError() != ERROR_ALREADY_EXISTS)
				return false;
		}

		char fileName[MAX_PATH] = {};

		SYSTEMTIME st;
		GetLocalTime(&st);

		HRESULT hr = StringCchPrintfA(fileName, MAX_PATH, "%s\\log_%04d_%02d_%s.txt",
			directory_, st.wYear, st.wMonth, fileDirectory);

		if (FAILED(hr))
			return false;

		auto& fout = logFiles_[fileDirectory];

		fout.open(fileName, std::ios::out | std::ios::app);

		return fout.is_open();
	}

	const char* GetLevelString(ELOG level)
	{
		switch (level)
		{
		case ELOG::DEBUG_LEVEL:
			return "DEBUG";

		case ELOG::ERROR_LEVEL:
			return "ERROR";

		case ELOG::SYSTEM_LEVEL:
			return "SYSTEM";

		default:
			return "UNKNOWN";
		}
	}

public:

	Logger() = default;

	~Logger()
	{
		std::lock_guard<std::mutex> lock(lock_);

		for (auto& [serverName, fout] : logFiles_)
		{
			if (fout.is_open())
			{
				fout.flush();
				fout.close();
			}
		}
	}

	void Initialize(const char* directory, ELOG_MODE mode)
	{
		if (directory == nullptr)
			return;

		std::lock_guard<std::mutex> lock(lock_);

		if (logFiles_.size() > 0)
		{
			__debugbreak();
			return;
		}

		HRESULT hr = StringCchCopyA(directory_, MAX_PATH, directory);

		if (FAILED(hr))
		{
			__debugbreak();
			return;
		}

		mode_ = mode;

		if (!CreateDirectoryA(directory_, nullptr))
		{
			if (GetLastError() != ERROR_ALREADY_EXISTS)
			{
				__debugbreak();
				return;
			}
		}
	}

	void SetMode(ELOG_MODE mode)
	{
		std::lock_guard<std::mutex> lock(lock_);
		mode_ = mode;
	}

	void Log(const char* fileName, ELOG level, const char* fmt, ...)
	{
		if (fileName == nullptr || fmt == nullptr)
			return;

		if (mode_ == ELOG_MODE::NONE_MODE)
			return;

		if (mode_ == ELOG_MODE::ERROR_MODE && level == ELOG::DEBUG_LEVEL)
			return;

		char userMessage[1024] = {};

		va_list args;
		va_start(args, fmt);

		HRESULT hr = StringCchVPrintfA(userMessage, _countof(userMessage), fmt, args);

		va_end(args);

		if (FAILED(hr))
		{
			StringCchCopyA(userMessage, _countof(userMessage), "Log message formatting failed.");
		}

		SYSTEMTIME st;
		GetLocalTime(&st);

		char finalMessage[1400] = {};

		hr = StringCchPrintfA(finalMessage, _countof(finalMessage), "[%s] [%04d-%02d-%02d %02d:%02d:%02d / %s] %s\n", fileName,
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, GetLevelString(level), userMessage);

		if (FAILED(hr))
			return;

		std::lock_guard<std::mutex> lock(lock_);

		auto iter = logFiles_.find(fileName);

		if (iter == logFiles_.end())
		{
			if (!OpenLogFile(fileName))
			{
				__debugbreak();
				return;
			}

			iter = logFiles_.find(fileName);
		}

		auto& fout = iter->second;

		fout << finalMessage;
		fout.flush();

		if (!fout)
		{
			__debugbreak();
		}
	}
};

inline Logger g_Logger;

#define LOG(fileName, level, ...)				g_Logger.Log(fileName, level, __VA_ARGS__)
#define LOG_INIT(directory, mode)				g_Logger.Initialize(directory, mode)
