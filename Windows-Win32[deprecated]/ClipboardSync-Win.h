#pragma once

#include "resource.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <cstdio>
#include <string>

#pragma comment(lib,"ws2_32.lib") 
#pragma warning(disable:26451) 

SOCKET sock_init(u_short PORT, const char * IP) {
    WSADATA WSAData;
    if (WSAStartup(MAKEWORD(2, 0), &WSAData) == SOCKET_ERROR)  //WSAStartup()函数对Winsock DLL进行初始化
    {
        printf("Socket initialize fail!\n");
    }
    SOCKET sock;                                            //客户端进程创建套接字
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR)  //创建流套接字（与服务端保持一致）
    {
        printf("Socket create fail!\n");
        WSACleanup();
    }

    struct sockaddr_in ClientAddr;                //sockaddr_in结构用来标识TCP/IP协议下的地址，可强制转换为sockaddr结构
    ClientAddr.sin_family = AF_INET;                //指Internet域
    ClientAddr.sin_port = htons(PORT);            //指定服务端所预留的端口
    ClientAddr.sin_addr.s_addr = inet_addr(IP);    //指定服务端所绑定的IP地址
    if (connect(sock, (LPSOCKADDR)&ClientAddr, sizeof(ClientAddr)) == SOCKET_ERROR)  //调用connect()函数，向服务器进程发出连接请求  
    {
        printf("Connect fail!\n");
        closesocket(sock);
        WSACleanup();
    }
    printf("Connected to %s:%u\n", IP, PORT);
    return sock;
}

int sock_send(SOCKET sock, const char * buff, int buff_len) {
    auto ret = send(sock, buff, buff_len, 0);    
    return ret;
}

int sock_close(SOCKET sock) {
    shutdown(sock, SD_SEND);
    closesocket(sock);
    return WSACleanup();
}

std::string GbkToUtf8(const char* src_str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, src_str, -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len + 1];
    memset(wstr, 0, len + 1);
    MultiByteToWideChar(CP_ACP, 0, src_str, -1, wstr, len);
    len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* str = new char[len + 1];
    memset(str, 0, len + 1);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
    std::string strTemp = str;
    if (wstr) delete[] wstr;
    if (str) delete[] str;
    return strTemp;
}

int clip_send(const char * ip, u_short port, const char* buff) {
    auto sock = sock_init(port, ip);
    if (sock == INVALID_SOCKET) {
        return -1;
    }
    auto buffer = GbkToUtf8(buff);
    if (sock_send(sock, buffer.c_str(), buffer.size()) == SOCKET_ERROR) {
        return -2;
    }
    sock_close(sock);
    printf("Connection to %s:%dclosed.\n", ip, port);
    return 0;
}