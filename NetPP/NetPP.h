#pragma once
#ifdef __linux__
#else
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <windows.h>
#endif
#pragma comment (lib, "Ws2_32.lib")

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <functional>

#define NETPP_CONNECTION_RESET -1
#define DEFAULT_CLIENT_REQUEST_MAGIC 0x4545
#define DEFAULT_SERVER_RESPONSE_MAGIC 0x6969
#define NETPP_DEFAULT_SERVER_PORT 9909

bool CheckWsa()
{
    SOCKET TestSock = INVALID_SOCKET;

    TestSock = socket(AF_INET, SOCK_DGRAM, 0);

    if (WSAGetLastError() == WSANOTINITIALISED && TestSock == INVALID_SOCKET)
    {
        return false;
    }

    if (TestSock != INVALID_SOCKET)
        closesocket(TestSock);

    return true;
}

struct ServerFindPacket
{
    unsigned short Magic = DEFAULT_CLIENT_REQUEST_MAGIC;
};

struct ServerFindResponse
{
    unsigned short Magic = DEFAULT_SERVER_RESPONSE_MAGIC;
};

struct ClientInfo
{
    std::string IpV4 = "";
    int Port = 0;
    SOCKET Sock = INVALID_SOCKET;
};

class NetPP
{
    std::thread Listener;
    std::thread Broadcast;
    SOCKET ListenSocket = INVALID_SOCKET;
    int Port = NETPP_DEFAULT_SERVER_PORT;

    bool IsServer = false;
    bool ExitListener = false;
    bool HasBeenInstantiated = false;
    bool IsListening = false;
    bool IsBroadcasting = false;

    void(*AcceptCb)(sockaddr_in*, SOCKET&) = nullptr;

    void ListenerThread()
    {
        if (!CheckWsa())
        {
            WSADATA WsaDat;
            int ErrCode = WSAStartup(MAKEWORD(2, 2), &WsaDat);

            if (ErrCode != 0)
            {
                std::cout << "WsaStartup failed: " + ErrCode << std::endl;
            }
        }

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
        ListenAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        ListenAddr.sin_port = htons(this->Port);

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

            SOCKET Client = INVALID_SOCKET;

            Client = accept(ListenSocket, (sockaddr*)&ClientAddrInfo, &len);

            if(this->AcceptCb != nullptr)
            {
                this->AcceptCb(&ClientAddrInfo, Client);
            }

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

    void BroadCastThread(const unsigned short Port, const SIZE_T SizeOfServerFindPacket, std::function<void(SOCKET&, sockaddr_in*, char*, int)> const& PacketRecvCb)
    {
        if (!CheckWsa())
        {
            WSADATA WsaDat;
            int ErrCode = WSAStartup(MAKEWORD(2, 2), &WsaDat);

            if (ErrCode != 0)
            {
                std::cout << "WsaStartup failed: " + ErrCode << std::endl;
            }
        }

        SOCKET ServerSock = socket(AF_INET, SOCK_DGRAM, 0);

        if (ServerSock == INVALID_SOCKET)
        {
            std::cout << "Failed to create socket " << std::to_string(WSAGetLastError()) << std::endl;
            return;
        }

        sockaddr_in ServerListen;
        ServerListen.sin_family = AF_INET;
        ServerListen.sin_port = htons(Port);
        ServerListen.sin_addr.s_addr = INADDR_ANY;

        int RetCode = bind(ServerSock, (sockaddr*)&ServerListen, sizeof(ServerListen));

        if (RetCode != 0)
        {
            std::cout << "Failed to bind to socket " << std::to_string(WSAGetLastError()) << std::endl;
            return;
        }

        this->IsBroadcasting = true;

        std::cout << "[NETPP] Broadcasting on port: " << Port << std::endl;

        while (true)
        {
            char* Buffer = new char[SizeOfServerFindPacket];
            sockaddr_in ClientAddr;
            int Sz = sizeof(ClientAddr);

            int RecvBytes = recvfrom(ServerSock, Buffer, SizeOfServerFindPacket, 0, (sockaddr*)&ClientAddr, &Sz);

            if (RecvBytes != SOCKET_ERROR)
            {
                PacketRecvCb(ServerSock, &ClientAddr, Buffer, RecvBytes);
            }
        }

        closesocket(ServerSock);

        this->IsBroadcasting = false;
    }

    public:
    std::vector<ClientInfo> ConnectedClients = {};
    SOCKET ClientSocket = INVALID_SOCKET;

    NetPP()
    {
        if (this->HasBeenInstantiated)
        {
            return;
        }

        std::vector<ClientInfo> ConnectedClients = {};
        SOCKET ClientSocket = INVALID_SOCKET;

        bool IsServer = false;
        bool ExitListener = false;
        SOCKET ListenSocket = INVALID_SOCKET;
        int Port = NETPP_DEFAULT_SERVER_PORT;
        bool HasBeenInstantiated = false;
        bool IsListening = false;
    }

    // if TimeToSearch is 0 will loop indefinetly
    void SearchLAN(const unsigned short Port, const SIZE_T SizeofPacketToRecv, char* PacketToSend, SIZE_T SizeOfPacketToSend, const long TimeToSearch, std::function<void(sockaddr_in*, char*, int)> const& PacketRecvCb)
    {
        if (!CheckWsa())
        {
            WSADATA WsaDat;
            int ErrCode = WSAStartup(MAKEWORD(2, 2), &WsaDat);

            if (ErrCode != 0)
            {
                std::cout << "WsaStartup failed: " + ErrCode << std::endl;
            }
        }


        SOCKET ClientSock = socket(AF_INET, SOCK_DGRAM, 0);
        sockaddr_in ClientAddr;
        ClientAddr.sin_family = AF_INET;
        ClientAddr.sin_port = htons(Port);
        ClientAddr.sin_addr.s_addr = INADDR_BROADCAST;

        bool EnableSockOpt = true;
        setsockopt(ClientSock, SOL_SOCKET, SO_BROADCAST, (char*)&EnableSockOpt, sizeof(bool));


        sendto(ClientSock, PacketToSend, SizeOfPacketToSend, 0, (sockaddr*)&ClientAddr, sizeof(ClientAddr));

        fd_set readSet;
        struct timeval Timeout;
        Timeout.tv_sec = 1;
        Timeout.tv_usec = 0;

        DWORD Start = GetTickCount64();

        char* Info = nullptr;
        Info = new char[SizeofPacketToRecv];

        while (true)
        {
            FD_ZERO(&readSet);
            FD_SET(ClientSock, &readSet);

            if (select(0, &readSet, NULL, NULL, &Timeout) > 0 && Info != nullptr) // Data available
            {
                sockaddr_in ServerAddr;

                int Sz = sizeof(ServerAddr);
                RtlZeroMemory(Info, SizeofPacketToRecv);

                int RecvBytes = recvfrom(ClientSock, Info, SizeofPacketToRecv, 0, (sockaddr*)&ServerAddr, &Sz);

                if (RecvBytes != SOCKET_ERROR)
                {
                    PacketRecvCb(&ServerAddr, Info, SizeofPacketToRecv);
                }
                else
                {
                    std::cout << "Error Receiving packet: " << WSAGetLastError() << std::endl;
                }
            }

            if ((GetTickCount64() - Start > TimeToSearch * 1000) && TimeToSearch != 0)
                break;
        }

        if (Info != nullptr)
            delete[] Info;

        closesocket(ClientSock);
    }

    // Meant to run on seperate thread
    void BroadCastServer(const unsigned short Port, const SIZE_T SizeOfServerFindPacket, std::function<void(SOCKET&, sockaddr_in*, char*, int)> const& PacketRecvCb)
    {
        Broadcast = std::thread(&NetPP::BroadCastThread, this, Port, SizeOfServerFindPacket, PacketRecvCb);
    }

    void CreateClient(unsigned short Port, std::string IP)
    {
        if (!CheckWsa())
        {
            WSADATA WsaDat;
            int ErrCode = WSAStartup(MAKEWORD(2, 2), &WsaDat);

            if (ErrCode != 0)
            {
                std::cout << "WsaStartup failed: " + ErrCode << std::endl;
            }
        }

        std::cout << "Connecting to server" << std::endl;
        // Create a SOCKET for cconnecting to server

        this->ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (ClientSocket == INVALID_SOCKET)
        {
            printf("socket create failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return;
        }

        sockaddr_in ClientAddr;

        memset(&ClientAddr, 0, sizeof(ClientAddr));
        ClientAddr.sin_family = AF_INET;
        ClientAddr.sin_addr.s_addr = inet_addr(IP.c_str());
        ClientAddr.sin_port = htons(this->Port);

        // Connect to server.
        int ErrCode = connect(this->ClientSocket, (sockaddr*)&ClientAddr, (int)sizeof(ClientAddr));
        if (ErrCode == SOCKET_ERROR)
        {
            printf("connect failed with error: %ld\n", WSAGetLastError());
            closesocket(ClientSocket);
            this->ClientSocket = INVALID_SOCKET;
            return;
        }

        this->HasBeenInstantiated = true;
        this->IsServer = false;
    }

    void CreateServer(bool StartListener, int Port, void (*PostAccecptCb)(sockaddr_in*, SOCKET&) = nullptr)
    {
        if (!CheckWsa())
        {
            WSADATA WsaDat;
            int ErrCode = WSAStartup(MAKEWORD(2, 2), &WsaDat);

            if (ErrCode != 0)
            {
                std::cout << "WsaStartup failed: " + ErrCode << std::endl;
                return;
            }
        }

        this->Port = Port;
        // Initialize DLL for use and checl if version is available
        this->IsServer = true;
        this->AcceptCb = PostAccecptCb;

        if (StartListener)
        {
            std::cout << "starting listener from class" << std::endl;
            this->Listener = std::thread(&NetPP::ListenerThread, this);
        }

        this->HasBeenInstantiated = true;
    }

    void StartListening()
    {
        if (this->IsServer)
        {
            Listener = std::thread(&NetPP::ListenerThread, this);
            this->IsListening = true;
        }
    }

    void StopListening()
    {
        if (this->IsListening)
        {
            this->ExitListener = true;
            closesocket(this->ListenSocket);
            this->Listener.join();
            this->ExitListener = false;
        }
        else
        {
            return;
        }
    }

    int RecvClient(SOCKET Target, void* Buffer, int Len)
    {
        int RetCode = recv(Target, (char*)Buffer, Len, 0);

        return RetCode;
    }

    int RecvTimeout(SOCKET Target, void* Buffer, SIZE_T Len, long TimeoutMilliSecs)
    {
        fd_set readSet;
        struct timeval Timeout;

        Timeout.tv_sec = 0;
        Timeout.tv_usec = 50;

        DWORD Start = GetTickCount64();
        while (true)
        {
            FD_ZERO(&readSet);
            FD_SET(Target, &readSet);

            int RecvBytes = 0;

            if (select(0, &readSet, NULL, NULL, &Timeout) > 0 && Buffer != nullptr) // Data available
            {
                sockaddr_in ServerAddr;

                int Sz = sizeof(ServerAddr);
                RtlZeroMemory(Buffer, Len);

                RecvBytes = recvfrom(Target, (char*)Buffer, Len, 0, (sockaddr*)&ServerAddr, &Sz);

                if (RecvBytes != SOCKET_ERROR)
                {
                    return RecvBytes;
                }
                else
                {
                    std::cout << "Error recieving: " << GetLastError() << " WSA: " << WSAGetLastError() << std::endl;
                    return RecvBytes;
                }
            }

            if ((GetTickCount64() - Start > TimeoutMilliSecs) || TimeoutMilliSecs == 0)
                break;
        }

        return SOCKET_ERROR;
    }

    int SendToClient(SOCKET Target, void* Buffer, int Len)
    {
        int RetCode = send(Target, (char*)Buffer, Len, 0);

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
            if (this->ClientSocket != INVALID_SOCKET)
                closesocket(this->ClientSocket);

            WSACleanup();
        }
    }
};
