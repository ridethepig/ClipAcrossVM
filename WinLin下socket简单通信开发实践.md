# Win/Lin下socket简单通信开发实践

## 为什么会写这一份笔记

鉴于国内垃圾的网络环境, 当然还有VMWare无比buggy的虚拟机增强功能, 导致我找不到宿主机-虚拟机共享剪贴板的优秀解决方案, 于是我打算自己写一个. 因为对网络编程的无比生疏, 在做的过程中遇到了很多麻烦.

## 环境

虚拟机: Ubuntu 20.04 on VMWare Workstation Player 16

宿主机: Windows10 2004 x64

## 先说Linux端

显然这个项目不是很难, 代码200行不到即可解决问题. 服务器就是直接魔改的网上随处可见的`epoll`实现socket服务器(正好实践一下`epoll`相关的知识), 完整代码就不放上来献丑了. 基本上还是很稳定的(因为也没有什么复杂度). 

贴一个半成品, 随意取用

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
//#include "wrap.h"

const int MAXLINE = 80;
const int SERV_PORT = 8842;
const int OPEN_MAX = 1024;

int main(int argc, char *argv[])
{
    int i, j;
	int fd_listen, fd_connect, fd_socket;
    int n_ready, epoll_fd, res;
    ssize_t n;
    char buffer_data[MAXLINE], buffer_ip[INET_ADDRSTRLEN];
    socklen_t client_socket_len;
    int clients[OPEN_MAX];
    struct sockaddr_in addr_client, addr_server;
    struct epoll_event epoll_ev, ep[OPEN_MAX];

    fd_listen = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&addr_server, sizeof(addr_server));
    addr_server.sin_family = AF_INET;
    addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_server.sin_port = htons(SERV_PORT);
    bind(fd_listen, (struct sockaddr *) &addr_server, sizeof(addr_server));
    listen(fd_listen, 20);
	
    for (i = 0; i < OPEN_MAX; i++) clients[i] = -1;
    int last_client = -1;
    epoll_fd = epoll_create(OPEN_MAX);
    if (epoll_fd == -1) {
    	printf("epoll fd create error.\n") ;
    	return 1;
	}
	
    epoll_ev.events = EPOLLIN; 
	epoll_ev.data.fd = fd_listen;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_listen, &epoll_ev) == -1) {
        printf("epoll ctl add failed on fd_listen\n");
        return 2;
    }
	
	printf("server started on port %d\n", SERV_PORT);
	
    for ( ; ; )
	{
        n_ready = epoll_wait(epoll_fd, ep, OPEN_MAX, -1); 
        if (n_ready == -1) {
            printf("epoll wait error.\n");
            return 3;
        }
		
        for (i = 0; i < n_ready; i++) 
		{	
            if ((ep[i].events & EPOLLIN) && ep[i].data.fd == fd_listen) 
			{
                client_socket_len = sizeof(addr_client);
                fd_connect = accept(fd_listen, (struct sockaddr *)&addr_client, &client_socket_len);
                printf("received from %s at PORT %d\n",
					inet_ntop(AF_INET, &addr_client.sin_addr, buffer_ip, sizeof(buffer_ip)), ntohs(addr_client.sin_port));
			    for (j = 0; j < OPEN_MAX; j++){
			    	if (clients[j] < 0) 
					{
						clients[j] = fd_connect;
						break;
					}
				}								                
				
                epoll_ev.events = EPOLLIN; 
				epoll_ev.data.fd = fd_connect;
				                
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_connect, &epoll_ev) == -1) {
                    printf("epoll ctl add connect fd %d failed. ignoring\n", fd_connect);
                    continue;
                }
                
                if (j > last_client) last_client = j;
            }
            else 
			{
                fd_socket = ep[i].data.fd;
                n = read(fd_socket, buffer_data, MAXLINE);
				
                if (n == 0) 
				{
                    for (j = 0; j <= last_client; j++) 
					{
                        if (clients[j] == fd_socket) 
						{
                            clients[j] = -1;
                            break;
                        } 
					}
					                    
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_socket, NULL) == -1) {
                        printf("epoll ctl del fd_socket %d failed. ignoring\n", fd_socket);                        
                    }
                    close(fd_socket);
                    printf("client[%d] closed connection\n", j);
                }
                else
				{
				  printf("Clipper says:"); fflush_unlocked(stdout);
                    write(1, buffer_data, n);
                    puts("");
                }
			}
		} 
	}
    close(fd_listen);
    close(epoll_fd);
    return 0;
}
```

主要就是剪切板的部分. 这里直接使用`xclip`的解决方案, 貌似如果自己写的话, 会比较麻烦, 因为会涉及到X11系统相关的各种又臭又长的API, 既然有人做好了, 就没有必要重新造轮子. 而且很多新写出来的终端编辑器(比如micro)也是使用了类似的解决方案. 于是先确保安装了`xclip` : `sudo apt install xclip`即可解决问题. 然后因为`xclip`貌似只能通过管道和文件向里面传数据, 于是就使用最简单的`popen`. 反正这玩意也不会有什么太大的并发量, 不需要考虑效率问题还真是十分快乐.说实在的, X11的瓜皮架构本身效率也不是很高. 代码如下:

```c
FILE* f_popen = popen("xclip -sel c", "w");
if (f_open == NULL) {
    printf("Error pipe into xclip,\n");
    continue;
}
fwrite(buffer, sizeof(char), n_read, f_open);
pclose(f_popen);
```

**提几点注意:**

1. `xclip`记得使用`-sel c`参数. 要不然会有奇怪的事情发生
2. `popen`打开的管道文件必须使用`pclose`来关闭, 否则会有大问题
3. 这个东西别拿`ssh`写. 因为X-Server默认是不允许服务器操作的. 所以`xclip`会写不进去. 不过实际用的时候也不会在服务器上用就是了.

然后调试socket发现一个神奇的小工具: Hercules. Google一下应该就有了

就先写这些东西吧. 

## 再说Windows 端

这部分写的比较曲折, 因为对Windows的网络不是很熟悉.

### Deprecated Win32版本

这个是一开始随便瞎做的, 因为不想用Win32像\*一般的古早系统写界面故最后放弃了. 不过成果还是贴一下.

这边是剪贴板监控的部分. 主要是抄的MSDN上面的东西. 基本上就是调用一组clipboard相关的API. Windows已经准备好一套剪贴板的监控机制, 把自己的程序加到消息链里面基本上就完事了. 中间还加了一点奇形怪状的写法(历史与现代的完美结合). 还有就是头一次见到那个`GlobalLock`,,,Windows真的是老古董.

```cpp
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static UINT auPriorityList[] = {
		   CF_OWNERDISPLAY,
		   CF_TEXT,
		   CF_ENHMETAFILE,
		   CF_BITMAP
	};
	static int uFormat;
	static HWND hwndNextViewer;
	static char buffer[4096];
	switch (message)
	{
	case WM_CREATE:
	{
		hwndNextViewer = SetClipboardViewer(hWnd);
	}
	break;
	case WM_DRAWCLIPBOARD:
		uFormat = GetPriorityClipboardFormat(auPriorityList, 4);
		if (uFormat == CF_TEXT) {
			if (OpenClipboard(hWnd))
			{
				auto hglb = GetClipboardData(uFormat);
				auto lpstr = GlobalLock(hglb);
				if (lpstr != nullptr) {
					strcpy(buffer, reinterpret_cast<char*>(lpstr));
					GlobalUnlock(hglb);
					printf("%s\n", buffer);
					std::async(clip_send, ip, port, buffer);
				}
				else {
					GlobalUnlock(hglb);
				}
				CloseClipboard();
			}
		}
		else {
			_tprintf(TEXT("%d\n"), uFormat);
		}
		SendMessage(hwndNextViewer, message, wParam, lParam);
		break;
	case WM_CHANGECBCHAIN:
		if ((HWND)wParam == hwndNextViewer)
			hwndNextViewer = (HWND)lParam; 
		else if (hwndNextViewer != NULL)
			SendMessage(hwndNextViewer, message, wParam, lParam);
		break;
	case WM_DESTROY:
		ChangeClipboardChain(hWnd, hwndNextViewer);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
```

然后是`Winsock2`的部分. 其实没啥东西, 直接抄的MSDN.

```cpp
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <cstdio>
#include <string>

#pragma comment(lib,"ws2_32.lib") 
#pragma warning(disable:26451) 

SOCKET sock_init(u_short PORT, const char * IP) {
    WSADATA WSAData;
    if (WSAStartup(MAKEWORD(2, 0), &WSAData) == SOCKET_ERROR)  
    {
        printf("Socket initialize fail!\n");
    }
    SOCKET sock;                                           
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR)
    {
        printf("Socket create fail!\n");
        WSACleanup();
    }

    struct sockaddr_in ClientAddr;                //½á¹¹ÓÃÀ
    ClientAddr.sin_family = AF_INET; 
    ClientAddr.sin_port = htons(PORT);
    ClientAddr.sin_addr.s_addr = inet_addr(IP); 
    if (connect(sock, (LPSOCKADDR)&ClientAddr, sizeof(ClientAddr)) == SOCKET_ERROR)
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
```

其中只有一个需要注意的地方, 就是编码问题. 困扰良久. 

众所周知Windows的字符编码与众不同, 特立独行. 看了半天发现它复制过来的东西, 居然是`GBK`的编码, 不是UTF-8. 所以得转一下编码. 这种就是瓜皮的历史遗留问题了. 要不然发到Linux下面会乱码, 那边统一都是UTF-8.

### Qt版本

Qt是真的方便

剪贴板直接使用Qt内置的`QClipboard`类即可, 自己加一个槽函数就可以了.

```cpp
connect(clipboard, SIGNAL(changed(QClipboard::Mode)), this, SLOT(clipboardChanged(QClipboard::Mode)));

void ClipClient::clipboardChanged(QClipboard::Mode mode) {
    if (!on_listen) return;
    QString str = clipboard->text();
    if (str == last_clip) return;
    QString ip = ui.lineEdit->text(); // I just don't want to waste my time validating it
    int port = ui.spinBox->value();
    QtConcurrent::run(this, &ClipClient::sendData, ip, port, str);
    last_clip = str;
}
```

然后就是socket的部分. Qt自己实现的socket是真的方便. 然而如果直接写的话, 会出现等待连接的时候的界面假死. 于是发请求的部分单独开一个线程. 相比较C++11实现的`async`, Qt的异步也没差多少. 使用起来同样非常简洁. 调用部分见上一段代码的`QtConcurrent::run`

```cpp
void ClipClient::sendData(QString ip, int port, QString data)
{    
    QTcpSocket* tcpSocket = new QTcpSocket();
    if (tcpSocket != nullptr) {
        tcpSocket->connectToHost(ip, port);
        if (tcpSocket->waitForConnected(3000)) {
            tcpSocket->write(data.toStdString().c_str());
            tcpSocket->flush();
            tcpSocket->disconnectFromHost();
            emit socket_complete_signal(0);
        }
        else {
            tcpSocket->disconnectFromHost();
            emit socket_complete_signal(1);
        }
    }
    else {
        emit socket_complete_signal(2);
    }
}
```

不过这里值得注意的是, 不要尝试跨线程操作对象. 必须坚持`RAII`原则, 尽量不使用引用, 也不能直接在子线程里面操作界面元素, 否则崩溃在所难免, 势在必行.

还有一个问题就是, 有时候请求连上了却发不出去. 这是因为缓冲区未满Qt的socket没把它写进去. 在这里我使用了短链接, 所以发完就直接`socket->flush()`一下, 否则到断开也有可能数据没有发出去. 还有就是这里断开连接是用`disconnectFromHost`不要一不小心补全成了`disconnected`......

在写这个项目的过程中, 还发现了Qt里面一个非常方便的东西, 叫做`QSettings`, 可以自动读写注册表, `INI`等类型的配置文件, 不需要手写解析了. 

```cpp
settings->beginGroup("conn_data");
settings->setValue("ip", ui.lineEdit->text());
settings->setValue("port", ui.spinBox->value());
settings->endGroup();
```

至于字符编码, 艹, Qt居然自己就是UTF-8, 还是Qt好, 不愧为当代C++跨平台界面开发的唯一选择.

此外我还实践了一下系统托盘, 可以作为参考.

大概就是这样.至此, 这一套系统就开发完成了. 总共历经三天. 1.3-1.5