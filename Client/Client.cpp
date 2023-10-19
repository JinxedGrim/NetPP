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
	bool UseCustomPacket = true; // it is absolutely possible to recv a custom packet with a size then recv another packet to determine this but i am lazy so a switch it is
	int Sz = 0;

	if (UseCustomPacket == true)
		Sz = sizeof(CustomServerFindResponse);
	else
		Sz = sizeof(ServerFindPacket);

	std::vector<std::string> Ips = {};

	const auto Cb = [&Ips](sockaddr_in* ServerInfo, char* Packet, int SizePacket)
	{
		if (((ServerFindResponse*)Packet)->Magic == DEFAULT_SERVER_RESPONSE_MAGIC || (((ServerFindResponse*)Packet)->Magic == CUSTOM_PACKET_MAGIC && SizePacket == sizeof(CustomServerFindResponse)))
		{
			std::string Ip = inet_ntoa(ServerInfo->sin_addr);
			
			if (((ServerFindResponse*)Packet)->Magic == CUSTOM_PACKET_MAGIC)
				Ip += " Clients(" + std::to_string(((CustomServerFindResponse*)Packet)->ConnectedClients) + ")";

			std::cout << "Found server: " << Ip << std::endl;

			Ips.push_back(Ip);
		}
		else
		{
			std::cout << "Invalid magic in packet: " << ((ServerFindResponse*)Packet)->Magic << " Magic 1: " << DEFAULT_SERVER_RESPONSE_MAGIC << " 2. " << CUSTOM_PACKET_MAGIC << std::endl;
		}
	};

	std::cout << "Searching for servers on LAN" << std::endl;

	ServerFindPacket PackToBroadcast;
	SearchLAN(9191, Sz, (char*)(&PackToBroadcast), sizeof(PackToBroadcast), 3, Cb);

	std::string Ip;

	if (Ips.size() == 0)
	{
		std::cout << "No servers found on LAN.\nPlease enter an IP: " << std::endl;
		std::cin >> Ip;
	}
	else
	{
		std::cout << "Select a server to connect to:\n";
		int Idx = 0;
		for (const std::string& _Ip : Ips)
		{
			system("cls");
			std::cout << Idx << ". " << _Ip << std::endl;
		}

		Idx = 0;
		std::cin >> Idx;

		Ip = Ips[Idx];
	}

	std::cout << "Attempting to connect to: " << Ip << std::endl;

	NetPP Client = NetPP(false, false, 23234, Ip);

	while (true)
	{
		char Msg[20];
		ZeroMemory(&Msg, sizeof(Msg));
		std::cout << "Enter message max buffer: " << sizeof(Msg) << std::endl;

		std::cin >> Msg;
		send(Client.ClientSocket, Msg, sizeof(Msg), 0);

		ZeroMemory(Msg, 20);

		int RetCode = recv(Client.ClientSocket, Msg, sizeof(Msg), 0);

		if (RetCode == NETPP_CONNECTION_RESET)
		{
			std::cout << "Disconnected from server" << std::endl;
		}

		std::cout << Msg << std::endl;
	}
}
