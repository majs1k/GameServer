#include <conio.h>
#include "TestServer.h"
#include "Utils/ConfigLoader.h"
#include "Utils/CrashDump.h"
#include "Utils/Logger.h"
#include "Utils/Profiler.h"

#define CONFIG_FILENAME			L"TestConfig.json"

int main()
{
	CrashDump().Initialize();
	LOG_INIT("./Log", ELOG_MODE::NONE_MODE);
	ConfigLoader::Instance().Load(CONFIG_FILENAME);

	TestServer server;
	server.Start(config.ip, config.port, config.sessionMax, config.concurrentCount, config.workerCount);

	while (1)
	{
		if (GetAsyncKeyState(VK_ESCAPE) & 0x0001)
		{
			__debugbreak();
		}

		//if (GetAsyncKeyState('X') & 0x0001)
		//	break;
	}

	server.Stop();
	return 0;
}
