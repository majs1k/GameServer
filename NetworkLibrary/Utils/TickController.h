// -------------------------------------------------------------------
// 
// 틱 컨트롤러
//
// ms 단위 계산
// 완벽한 고정프레임 불가 -> delteTime 변수 필요 (로직에서 활용)
// 프레임 밀리면 다음 구간에서 보정
// 
// cf. 패킷(메시지) TPS와 DB TPS 별개
//
// -------------------------------------------------------------------
#pragma once
#pragma comment(lib, "winmm.lib")
#include <iostream>
#include "Singleton.h"
#include <Windows.h>

#define TARGET_FPS		  50

class TickController : public Singleton<TickController>
{
private:

	DWORD frameStartTime_;
	DWORD curTime_;
	DWORD fpsStartTime_;
	int deltaTime_;

	int curFps_;
	int frameCount_;

public:

	TickController()
	{
		// 시간 해상도 높여야 정확한 Sleep 가능
		timeBeginPeriod(1);

		DWORD time = timeGetTime();
		frameStartTime_ = time;
		curTime_ = time;
		deltaTime_ = 0;

		fpsStartTime_ = time;
		curFps_ = 0;
		frameCount_ = 0;
	}

	DWORD CurTime() const
	{
		return curTime_;
	}

	float DeltaTime() const
	{
		return deltaTime_;
	}

	int Fps() const
	{
		return curFps_;
	}

	void Update()
	{
		DWORD oldTime = curTime_;
		curTime_ = timeGetTime();

		deltaTime_ = curTime_ - oldTime;

		int frameRunTime = curTime_ - frameStartTime_;

		// 시간차는 int형으로 선언
		int frameSpareTime = CLOCKS_PER_SEC / TARGET_FPS - frameRunTime;

		if (frameSpareTime > 0)
		{
			Sleep(frameSpareTime);
		}

		frameStartTime_ += CLOCKS_PER_SEC / TARGET_FPS;

		frameCount_++;

		if (CLOCKS_PER_SEC <= curTime_ - fpsStartTime_)
		{
			fpsStartTime_ += CLOCKS_PER_SEC;

			curFps_ = frameCount_;
			frameCount_ = 0;
		}
	}
};
