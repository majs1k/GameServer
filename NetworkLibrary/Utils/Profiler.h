// -------------------------------------------------------------------
// 
// 프로파일러
// 
// QueryPerformanceCounter로 인자 전달한 후 QuadPart로 시간 추출
// Profile 클래스 멤버변수들은 100ns 단위 (정밀함을 위함)
// 파일 출력은 us단위로 진행
// 
// 1. 시간을 측정해서 로직 개선 / 자료구조 수정
// 2. 호출횟수를 측정해서 자주 호출되는 함수 성능 개선 (80-20 법칙)
// 
// 스페이스바 -> 파일 저장
// C키 -> 리셋
//
// 프로그램 실행 후, 프로파일러 리셋 1회 권장
// 
// -------------------------------------------------------------------
#pragma once
#pragma comment(lib, "winmm.lib")
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <conio.h>
#include <Windows.h>


struct ProfileData
{
	std::wstring		name_;

	__int64				totalTime_;
	__int64				minTime_;
	__int64				maxTime_;
	__int64				call_;

	LARGE_INTEGER		startTime_;

public:

	ProfileData(const std::wstring& name)
		:name_(name), totalTime_(0), minTime_(INT_MAX), maxTime_(0), call_(0), startTime_{}
	{
	}

	void Update(__int64 time)
	{
		totalTime_ += time;
		call_++;

		if (time < minTime_)
			minTime_ = time;

		if (maxTime_ < time)
			maxTime_ = time;
	}

	void Clear()
	{
		totalTime_ = 0;
		minTime_ = INT_MAX;
		maxTime_ = 0;
		call_ = 0;
	}
};

class Profile
{
private:

	std::unordered_map<std::wstring, ProfileData*> profileMap_;

	LARGE_INTEGER freq_;

	bool isInit_ = false;

	std::mutex lock_;

public:

	Profile()
	{
		timeBeginPeriod(1);

		// 1초의 진동주기 (현재 os에서는 천만 -> 100ns 단위 측정 가능)
		QueryPerformanceFrequency(&freq_);
	}

	~Profile()
	{
		for (auto& p : profileMap_)
		{
			delete p.second;
		}
	}

	bool IsInit()
	{
		return isInit_;
	}

	void Init()
	{
		isInit_ = true;
	}

	void Clear()
	{
		lock_.lock();

		for (auto& p : profileMap_)
		{
			p.second->Clear();
		}

		lock_.unlock();
	}

	ProfileData* Find(const std::wstring& name)
	{
		auto it = profileMap_.find(name);

		if (it != profileMap_.end())
			return (*it).second;
		else
			return nullptr;
	}

	void BeginProfile(const std::wstring& name)
	{
		// Profiler 리스트에서 들고 오거나, 없다면 리스트에 추가
		ProfileData* profile = this->Find(name);

		if (profile == nullptr)
		{
			profile = new ProfileData(name);

			profileMap_.insert({ profile->name_, profile });
		}

		QueryPerformanceCounter(&(profile->startTime_));
	}

	void EndProfile(const std::wstring& name)
	{
		LARGE_INTEGER endTime;
		QueryPerformanceCounter(&endTime);

		ProfileData* profile = this->Find(name);

		if ((endTime.QuadPart - profile->startTime_.QuadPart) < 0)
		{
			__debugbreak();
		}

		// 100ns 단위 환산
		__int64 timeDiff = (__int64)((endTime.QuadPart - profile->startTime_.QuadPart) * 10'000'000 / freq_.QuadPart);

		lock_.lock();

		profile->Update(timeDiff);

		lock_.unlock();
	}

	void Save()
	{
		if (profileMap_.empty())
			return;

		SYSTEMTIME st;
		GetLocalTime(&st);

		WCHAR fileName[MAX_PATH];
		swprintf_s(fileName, MAX_PATH, L"./Log/profiler_%04d%02d%02d_%02d%02d%02d.txt",
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

		std::wofstream fout(fileName, std::ios::app);

		if (!fout.is_open())
		{
			printf("logfile open error\n");
			printf("check file directory\n");

			__debugbreak();
		}

		fout << std::setw(15) << L"Name"
			<< std::setw(15) << L"Average(us)"
			<< std::setw(15) << L"Min(us)"
			<< std::setw(15) << L"Max(us)"
			<< std::setw(15) << L"Call"
			<< std::endl;

		fout << L"--------------------------------------------------------------------------------------" << std::endl;

		for (auto& p : profileMap_)
		{
			ProfileData* profile = p.second;

			if (profile->call_ <= 2)
				continue;

			fout << std::fixed << std::setprecision(3)
				<< std::setw(15) << profile->name_
				<< std::setw(15) << static_cast<double>(profile->totalTime_ - profile->minTime_ - profile->maxTime_)
				/ 10.0 / (profile->call_ - 2)
				<< std::setw(15) << static_cast<double>(profile->minTime_) / 10
				<< std::setw(15) << static_cast<double>(profile->maxTime_) / 10
				<< std::setw(15) << profile->call_
				<< std::endl << std::endl;
		}

		fout.close();
	}
};

class Profiler
{
private:

	inline static std::vector<Profile*> profilers_;

	inline static std::mutex lock_;

	inline static std::thread t_;

public:

	static void Start()
	{
		t_ = std::thread(&Profiler::ProfilerThread);
	}

	static void Stop()
	{
		if (Profiler::t_.joinable())
		{
			Profiler::t_.join();
		}
	}

	static Profile& GetProfiler()
	{
		static thread_local Profile profiler;

		if (!profiler.IsInit())
		{
			lock_.lock();

			profilers_.push_back(&profiler);

			lock_.unlock();

			profiler.Init();
		}

		return profiler;
	}

	static void Save()
	{
		lock_.lock();

		for (auto& p : profilers_)
			p->Save();

		lock_.unlock();

		wprintf(L"profiler saved\n");
	}

	static void Clear()
	{
		lock_.lock();

		for (auto& p : profilers_)
			p->Clear();

		lock_.unlock();

		wprintf(L"profiler clear\n");
	}

	static void ProfilerThread()
	{
		while (1)
		{
			int c = _getch();

			if (c == ' ')
			{
				Profiler::Save();
			}

			else if (c == 'c')
			{
				Profiler::Clear();
			}

			else if (c == 'x')
			{
				Profiler::Stop();

				break;
			}
		}
	}
};

// RAII
class ProfileGuard
{
private:
	std::wstring name_;

public:
	ProfileGuard(const std::wstring& name)
	{
		name_ = name;
		Profiler::GetProfiler().BeginProfile(name_);
	}

	~ProfileGuard()
	{
		Profiler::GetProfiler().EndProfile(name_);
	}
};



#define PROFILER

#ifdef PROFILER

#define PRO(name)				ProfileGuard pg(name)
#define PRO_BEGIN(name)			Profiler::GetProfiler().BeginProfile(name)
#define PRO_END(name)			Profiler::GetProfiler().EndProfile(name)

#else
#define PRO(name)
#define PRO_BEGIN(name)
#define PRO_END(name)

#endif
