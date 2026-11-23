#pragma comment(lib, "winmm.lib")

#include <iostream>

#include "Database/Database.h"
#include "DBHeader.h"
#include "DBserver.h"

int main()
{
	timeBeginPeriod(1);

	DBServer server;

	server.Start();

	while (1)
	{
		if (GetAsyncKeyState(VK_ESCAPE) & 0x0001)
		{
			__debugbreak();
			break;
		}

		if (GetAsyncKeyState('1') & 0x0001)
		{
			server.SetItemTradeMode(1);
		}

		if (GetAsyncKeyState('2') & 0x0001)
		{
			server.SetItemTradeMode(2);
		}

		if (GetAsyncKeyState('3') & 0x0001)
		{
			server.SetItemTradeMode(3);
		}

		server.Monitoring();

		Sleep(1000);
	}

	return 0;
}
