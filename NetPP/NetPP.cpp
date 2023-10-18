#include <iostream>
#include "Networking/NetPP.h"

int main()
{
	const auto Cb = [](SOCKET ServerSock, sockaddr_in* ClientInfo, char* Packet)
	{
		if (((ServerFindPacket*)Packet)->Magic == DEFAULT_CLIENT_REQUEST_MAGIC)
		{
			ServerFindResponse sfr;
			sendto(ServerSock, (char*)&sfr, sizeof(sfr), 0, (sockaddr*)ClientInfo, sizeof(sockaddr_in));
		}
	};

	std::thread t(BroadCastServer, 9191, sizeof(ServerFindPacket), Cb);

	NetPP Server = NetPP(true, true, 23234);

	while (!GetAsyncKeyState(VK_ESCAPE))
	{
		for (int i = 0; i < Server.ConnectedClients.size(); i++)
		{
			std::cout << "Waiting for data from client: " << Server.ConnectedClients[i].IpV4 << std::endl;
			char Buffer[20];
			
			if (Server.RecvFromClient(Server.ConnectedClients[i].Sock, Buffer, 20) == NETPP_CONNECTION_RESET)
			{
				std::cout << "Client: " << Server.ConnectedClients[i].IpV4 << " disconnected" << std::endl;
				Server.ConnectedClients.erase(Server.ConnectedClients.begin() + i);
			}

			send(Server.ConnectedClients[i].Sock, Buffer, 20, 0);
		}
	}

	Server.Shutdown();
}
