#include <iostream>
#include "Networking/NetPP.h"

#define CUSTOM_PACKET_MAGIC 0x3939

struct CustomServerFindResponse
{
	unsigned short Magic = CUSTOM_PACKET_MAGIC;
	unsigned short ConnectedClients = 0;
};

int main()
{
	NetPP Server = NetPP(true, true, 23234);

	ServerFindResponse sfr;
	CustomServerFindResponse Csfr;
	int Sz = sizeof(Csfr);
	char* Packet = (char*)&Csfr;

	const auto Cb = [&Server, Packet, Sz](SOCKET& ServerSock, sockaddr_in* ClientInfo, char* RecvPacket, int SizePacket)
	{
		if (((ServerFindPacket*)RecvPacket)->Magic == DEFAULT_CLIENT_REQUEST_MAGIC)
		{
			if (Sz > sizeof(ServerFindPacket))
			{
				((CustomServerFindResponse*)Packet)->ConnectedClients = Server.ConnectedClients.size();
			}


			if (sendto(ServerSock, Packet, Sz, 0, (sockaddr*)ClientInfo, sizeof(*ClientInfo)) == SOCKET_ERROR)
			{
				std::cout << "Failed to send packet: " << WSAGetLastError() << std::endl;
			}
		}
		else
		{
			std::cout << "Invalid magic in packet: " << ((ServerFindPacket*)Packet)->Magic << std::endl;
		}
	};

	std::thread t(BroadCastServer, 9191, sizeof(ServerFindPacket), Cb);

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
