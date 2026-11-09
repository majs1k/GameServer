#include <conio.h>
#include "ChatServer.h"
#include "Utils/ConfigLoader.h"
#include "Utils/CrashDump.h"
#include "Utils/Profiler.h"

#define CONFIG_FILENAME			L"ChatConfig.json"

int main()
{
	CrashDump().Initialize();

	ConfigLoader::Instance().Load(CONFIG_FILENAME);

	Profiler::Start();

	ChatServer server;

	server.Start(config.ip, config.port, config.sessionMax, config.concurrentCount, config.workerCount);


	while (1)
	{
		if (GetAsyncKeyState(VK_ESCAPE) & 0x0001)
		{
			__debugbreak();
			break;
		}

		//if (GetAsyncKeyState('X') & 0x0001)
		//	break;

		//else if (GetAsyncKeyState(VK_SPACE) & 0x0001)
		//	ProfilerManager::Instance().Save();

		//else if (GetAsyncKeyState('C') & 0x0001)
		//	ProfilerManager::Instance().Clear();

		Sleep(1000);
	}

	server.Stop();

	return 0;
}
