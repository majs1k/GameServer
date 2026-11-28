#include <conio.h>
#include "LoginServer.h"
#include "Utils/ConfigLoader.h"
#include "Utils/CrashDump.h"
#include "Utils/Logger.h"
#include "Utils/Profiler.h"

#define CONFIG_FILENAME			L"loginconfig.json"

int main()
{
	CrashDump().Initialize();
	ConfigLoader::Instance().Load(CONFIG_FILENAME);
	LOG_INIT("./Log", ELOG_MODE::ERROR_MODE);

	LoginServer server;
	server.Start(config.ip, config.port, config.sessionMax, config.concurrentCount, config.workerCount);

	while (1)
	{
		if (GetAsyncKeyState(VK_ESCAPE) & 0x0001)
		{
			__debugbreak();
			break;
		}

		//else if (GetAsyncKeyState('X') & 0x0001)
		//	break;

		Sleep(1000);
	}

	server.Stop();
	return 0;
}
