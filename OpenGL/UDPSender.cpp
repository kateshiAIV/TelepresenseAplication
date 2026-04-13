#include <winsock2.h>
#include "UDPSender.h"
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <string>

void UDPSender::sendData()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5005);
    addr.sin_addr.s_addr = InetPton(AF_INET, TEXT("127.0.0.1"), &addr.sin_addr);; // Quest IP

    std::string message = "1.0 2.0 3.0 1.0 0.0 0.0"; // x y z r g b

    sendto(sock, message.c_str(), message.size(), 0,
        (sockaddr*)&addr, sizeof(addr));

    closesocket(sock);
    WSACleanup();
}