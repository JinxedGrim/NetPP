#pragma once
#ifdef __linux__
#else
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <windows.h>
#endif
#pragma comment (lib, "Ws2_32.lib")

#include <vector>
#include <string>
#include <thread>

#define NETPP_CONNECTION_RESET -1

struct ClientInfo
{
    std::string IpV4 = "";
    int Port = 0;
    SOCKET Sock = INVALID_SOCKET;
};

class NetPP
{
	bool IsServer = false;
	std::thread Listener;
    bool ExitListener = false;
    SOCKET ListenSocket = INVALID_SOCKET;

    void ListenerThread()
    {

        // Create a SOCKET for the server to listen for client connections.
        this->ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (ListenSocket == INVALID_SOCKET)
        {
            std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
            return;
        }

        sockaddr_in ListenAddr;

        memset(&ListenAddr, 0, sizeof(ListenAddr));
        ListenAddr.sin_family = AF_INET;
        ListenAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        ListenAddr.sin_port = htons(9093);

        int ErrCode = bind(ListenSocket, (sockaddr*)&ListenAddr, (int)sizeof(ListenAddr));

        if (ErrCode == SOCKET_ERROR)
        {
            std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
            return;
        }

        ErrCode = listen(ListenSocket, SOMAXCONN);
        if (ErrCode == SOCKET_ERROR)
        {
            std::cout << "listen failed with error: " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
            return;
        }

        while (true)
        {
            sockaddr_in ClientAddrInfo;
            int len = sizeof(ClientAddrInfo);
            memset(&ClientAddrInfo, 0, sizeof(ClientAddrInfo));

            SOCKET Client = accept(ListenSocket, (sockaddr*)&ClientAddrInfo, &len);

            if (Client == INVALID_SOCKET)
            {
                if (this->ExitListener)
                {
                    break;
                }
            }
            else
            {
                ClientInfo Inf;

                Inf.IpV4 = inet_ntoa(ClientAddrInfo.sin_addr);
                Inf.Port = (int)ntohs(ClientAddrInfo.sin_port);
                Inf.Sock = Client;

                std::cout << "Client Connected: " << Inf.IpV4 << " Port: " << Inf.Port << std::endl;

                this->ConnectedClients.push_back(Inf);
            }
        }
    }

    public:
    std::vector<ClientInfo> ConnectedClients = {};
    SOCKET ClientSocket = INVALID_SOCKET;

	NetPP(bool IsServer, bool StartListener)
	{
		// Initialize DLL for use and checl if version is available
		this->IsServer = IsServer;
		WSADATA WsaDat;
		int ErrCode = WSAStartup(MAKEWORD(2, 2), &WsaDat);

		if (ErrCode != 0)
		{
			throw("Scoket Err: " + std::to_string(ErrCode));
		}

		if (this->IsServer)
		{
            if (StartListener)
            {
                std::cout << "starting listener from class" << std::endl;
                Listener = std::thread(&NetPP::ListenerThread, this);
            }
		}
        else
        {
            std::cout << "Connecting to server" << std::endl;
            // Create a SOCKET for connecting to server

            this->ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

            if (ClientSocket == INVALID_SOCKET)
            {
                printf("socket create failed with error: %ld\n", WSAGetLastError());
                WSACleanup();
            }

            sockaddr_in ClientAddr;

            memset(&ClientAddr, 0, sizeof(ClientAddr));
            ClientAddr.sin_family = AF_INET;
            ClientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
            ClientAddr.sin_port = htons(9093);

            // Connect to server.
            ErrCode = connect(ClientSocket, (sockaddr*)&ClientAddr, (int)sizeof(ClientAddr));
            if (ErrCode == SOCKET_ERROR) 
            {
                printf("connect failed with error: %ld\n", WSAGetLastError());
                closesocket(ClientSocket);
                ClientSocket = INVALID_SOCKET;
            }
        }
	}

    void StartListening()
    {
        if (this->IsServer)
        {
            Listener = std::thread(&NetPP::ListenerThread, this);
        }
    }

    void StopListening()
    {
        this->ExitListener = true;
        closesocket(this->ListenSocket);
        this->Listener.join();
        this->ExitListener = false;
    }

    int RecvFromClient(SOCKET Target, void* Buffer, int Len)
    {
        int RetCode = recv(Target, (char*)Buffer, Len, 0);

        return RetCode;
    }

    void Shutdown()
    {
        if (this->IsServer && this->ListenSocket != INVALID_SOCKET)
        {
            this->ExitListener = true;
            closesocket(this->ListenSocket);
            this->Listener.join();
            this->ExitListener = false;
            WSACleanup();
        }
        else
        {
            closesocket(this->ClientSocket);
            WSACleanup();
        }
    }
};