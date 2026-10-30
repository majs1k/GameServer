#include <conio.h>
#include "TestClient.h"

#define SERVER_IP			L"127.0.0.1"
#define SERVER_PORT			6000

int main()
{
	TestClient client;

	client.Connect(SERVER_IP, SERVER_PORT);

	while (1)
	{
		Sleep(1000);

		client.SendTestNum();
	}

	return 0;
}
