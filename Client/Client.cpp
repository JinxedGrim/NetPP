#include <iostream>
#include "Networking/NetPP.h"



int main()
{
	std::vector<std::string> Ips = {};

	const auto Cb = [&Ips](sockaddr_in* ServerInfo, char* Packet)
	{
		if (((ServerFindResponse*)Packet)->Magic == DEFAULT_SERVER_RESPONSE_MAGIC)
		{
			std::string Ip = inet_ntoa(ServerInfo->sin_addr);
			Ips.push_back(Ip);
		}
	};

	std::cout << "Searching for servers on LAN" << std::endl;

	SearchLAN(9191, sizeof(ServerFindResponse), 5, Cb);

	std::string Ip;

	if (Ips.size() == 0)
	{
		std::cout << "No servers found on LAN.\nPlease enter an IP: " << std::endl;
		std::cin >> Ip;
	}
	else
	{
		std::string Ip = Ips.at(0);
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
