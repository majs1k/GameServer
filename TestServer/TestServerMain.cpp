#include <conio.h>
#include "TestServer.h"
#include "Utils/ConfigLoader.h"
#include "Utils/CrashDump.h"
#include "Utils/Profiler.h"

#define CONFIG_FILENAME			L"TestConfig.json"

int main()
{
	CrashDump().Initialize();

	Profiler::Start();

	ConfigLoader::Instance().Load(CONFIG_FILENAME);

	TestServer server;

	server.Start(config.ip, config.port, config.sessionMax, config.concurrentCount, config.workerCount);

	while (1)
	{
		Sleep(1000);

		//if (GetAsyncKeyState('X') & 0x0001)
		//{
		//	break;
		//}

		//server.Monitoring();
	}

	server.Stop();

	Profiler::Stop();

	return 0;
}
