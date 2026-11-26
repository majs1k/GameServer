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

#define PROFILER

#ifdef PROFILER

#define PRO(name)					ProfileGuard guard(name)
#define PRO_BEGIN(name)				g_Profiler.BeginProfile(name)
#define PRO_END(name)				g_Profiler.EndProfile(name)

#else
#define PRO(name)
#define PRO_BEGIN(name)
#define PRO_END(name)

#endif


class Profiler
{
private:

	struct Sample
	{
		std::string name_;
		int64_t totalTime_ = 0;
		int64_t minTime_ = INT_MAX;
		int64_t maxTime_ = 0;
		int64_t call_ = 0;
		LARGE_INTEGER startTime_;

	public:

		Sample(const std::string& name)
			:name_(name)
		{
		}

		void Update(int64_t time)
		{
			totalTime_ += time;
			call_++;

			if (time < minTime_)
			{
				minTime_ = time;
			}

			if (maxTime_ < time)
			{
				maxTime_ = time;
			}
		}

		void Clear()
		{
			totalTime_ = 0;
			minTime_ = INT_MAX;
			maxTime_ = 0;
			call_ = 0;
		}
	};

	struct Profile
	{
	private:

		std::unordered_map<std::string, Sample*> profileMap_;
		LARGE_INTEGER freq_;

	public:

		Profile(Profiler* profiler)
		{
			// 1초의 진동주기 (현재 os에서는 천만 -> 100ns 단위 측정 가능)
			QueryPerformanceFrequency(&freq_);

			{
				std::lock_guard<std::mutex> lock(profiler->lock_);

				profiler->profilers_.push_back(this);
			}
		}

		~Profile()
		{
			for (auto& p : profileMap_)
			{
				delete p.second;
			}
		}

		void Clear()
		{
			for (auto& p : profileMap_)
			{
				p.second->Clear();
			}
		}

		// Profile 리스트에서 들고 오거나, 없다면 추가
		Sample* GetSample(const std::string& name)
		{
			auto it = profileMap_.find(name);

			if (it != profileMap_.end())
			{
				return it->second;
			}
			else
			{
				Sample* sample = new Sample(name);
				profileMap_.insert({ name, sample });
				return sample;
			}
		}

		void BeginProfile(const std::string& name)
		{
			Sample* sample = GetSample(name);
			QueryPerformanceCounter(&(sample->startTime_));
		}

		void EndProfile(const std::string& name)
		{
			LARGE_INTEGER endTime;
			QueryPerformanceCounter(&endTime);

			Sample* sample = GetSample(name);

			// 100ns 단위 환산
			int64_t timeDiff = (int64_t)((endTime.QuadPart - sample->startTime_.QuadPart) * 10'000'000 / freq_.QuadPart);
			sample->Update(timeDiff);
		}

		void Save()
		{
			if (profileMap_.empty())
				return;

			SYSTEMTIME st;
			GetLocalTime(&st);

			WCHAR fileName[MAX_PATH];
			swprintf_s(fileName, MAX_PATH, L"profiler_%04d%02d%02d_%02d%02d%02d.txt",
				st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

			std::ofstream fout(fileName);

			if (!fout.is_open())
			{
				printf("file directory error\n");
				__debugbreak();
			}

			fout << std::setw(15) << "Name"
				<< std::setw(15) << "Average(us)"
				<< std::setw(15) << "Min(us)"
				<< std::setw(15) << "Max(us)"
				<< std::setw(15) << "Call"
				<< std::endl;

			fout << "--------------------------------------------------------------------------------------" << std::endl;

			for (auto& p : profileMap_)
			{
				Sample* sample = p.second;

				if (sample->call_ <= 2)
					continue;

				fout << std::fixed << std::setprecision(3)
					<< std::setw(15) << sample->name_
					<< std::setw(15) << static_cast<double>(sample->totalTime_ - sample->minTime_ - sample->maxTime_)
					/ 10.0 / (sample->call_ - 2)
					<< std::setw(15) << static_cast<double>(sample->minTime_) / 10
					<< std::setw(15) << static_cast<double>(sample->maxTime_) / 10
					<< std::setw(15) << sample->call_
					<< std::endl << std::endl;
			}

			fout.close();
		}
	};

private:

	std::vector<Profile*> profilers_;
	std::thread commandThread_;
	std::mutex lock_;
	bool shutdown_ = false;

public:

	Profiler()
	{
		commandThread_ = std::thread(&Profiler::ProfilerThread, this);

		timeBeginPeriod(1);
	}

	~Profiler()
	{
		shutdown_ = true;

		if (Profiler::commandThread_.joinable())
		{
			Profiler::commandThread_.join();
		}
	}

	Profile& GetProfiler()
	{
		static thread_local Profile profile{ this };
		return profile;
	}

	void BeginProfile(const std::string& name)
	{
		GetProfiler().BeginProfile(name);
	}

	void EndProfile(const std::string& name)
	{
		GetProfiler().EndProfile(name);
	}

	void Save()
	{
		{
			std::lock_guard<std::mutex> lock(lock_);

			for (auto& p : profilers_)
			{
				p->Save();
			}
		}

		printf("profiler saved\n");
	}

	void Reset()
	{
		{
			std::lock_guard<std::mutex> lock(lock_);

			for (auto& p : profilers_)
			{
				p->Clear();
			}
		}

		printf("profiler reset\n");
	}

	static void ProfilerThread(Profiler* profiler)
	{
		while (!profiler->shutdown_)
		{
			int c = _getch();

			if (c == ' ')
			{
				profiler->Save();
			}

			else if (c == 'c')
			{
				profiler->Reset();
			}
		}
	}
};

inline Profiler g_Profiler;

// RAII
class ProfileGuard
{
private:

	std::string name_;

public:

	ProfileGuard(const std::string& name)
	{
		name_ = name;
		g_Profiler.BeginProfile(name_);
	}

	~ProfileGuard()
	{
		g_Profiler.EndProfile(name_);
	}
};
