//////////////////////////////////////////////////////////
// TCPClient.cpp文件

#include "assert.h"
#include "initSock.h"
#include <stdio.h>
CInitSock initSock;		// 初始化Winsock库

SOCKET s;
long volatile nSendAll = 0;
long volatile nRecvAll = 0;

#include "CommonInclude/Tools/FastQueue.h"

struct T
{
	T* pNext;
};
MutliPriorityQueue<T> queue;


DWORD WINAPI ThreadProc(__in  LPVOID lpParameter)
{
	queue.IsUsed();
	T a;
	queue.Push(&a, 1);
	queue.Pop();
	queue.Lock();
	queue.Unlock();
	queue.SetUsed(TRUE);
	return 0;
}


int main(int nArg, CHAR* Arg[])
{
	int iRet;
	long port;

	// 填写远程地址信息
	sockaddr_in servAddr; 
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(4567);
	// 注意，这里要填写服务器程序（TCPServer程序）所在机器的IP地址
	// 如果你的计算机没有联网，直接使用127.0.0.1即可
	servAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");


	if ( nArg >= 2)
	{
		servAddr.sin_addr.S_un.S_addr = inet_addr(Arg[1]);
	}
	if ( nArg >= 3)
	{
		port = strtol(Arg[2], 0, 10);
		if ( port > 0 && port < 65535)
		{
			servAddr.sin_port = htons((USHORT)port);
		}
	}

	HANDLE hEvt = CreateEvent(0, TRUE, FALSE, "TestTcpClient.Limb.1");
	assert(hEvt);
	DWORD dwErr = GetLastError();
	if ( dwErr !=  ERROR_ALREADY_EXISTS)
	{
		Sleep(12000);
		SetEvent(hEvt);
		Sleep(1);
	}
	else
		WaitForSingleObject(hEvt, 15000);

	DWORD dwTimeTick = GetTickCount();

	for ( int j = 0; j < 200; j++)
	{
		// 创建套节字
		s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(s == INVALID_SOCKET)
		{
			printf(" Failed socket() \n");
			ASSERT(0);
			return 0;
		}

		unsigned   long   sendbufsize = 0;
		int size = sizeof(sendbufsize);
		iRet = getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&sendbufsize, &size);
		sendbufsize = 64*1024;
		iRet = setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&sendbufsize, size);

		// 也可以在这里调用bind函数绑定一个本地地址
		// 否则系统将会自动安排

		if(::connect(s, (sockaddr*)&servAddr, sizeof(servAddr)) == -1)
		{
			printf(" Failed connect() \n");
			ASSERT(0);
			return 0;
		}

		for (int x=0; x<100; x++)
		{
			//发送数据
			nSendAll = 0;
			char SendBuff[] = "[test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str]";
			for (int i = 0; i<5 && nSendAll < 3400; i++)
			{
				DWORD dwWantSend = rand()%2999 + 1;
				int nSend = ::send(s, SendBuff, dwWantSend, 0);
				assert(nSend == dwWantSend);
				nSendAll += nSend;
				//Sleep(0);
			}

#define TIMES			10
			// 接收数据
			nRecvAll = 0;
			while ( nRecvAll < nSendAll*TIMES)
			{
				char buff[3001];
				int nRecv = ::recv(s, buff, 3000, 0);
				if(nRecv > 0)
				{
					buff[nRecv] = '\0';
					//printf(" 接收到数据(%d-%d)：[%s]\n", j, nRecv, buff);
					nRecvAll += nRecv;
				}
				else
				{
					printf("error!!! Err:%d", GetLastError());
					ASSERT(0);
					nRecvAll = nSendAll = 0;
					break;
				}
			}

			if ( nSendAll == 0)
				break;

			if ( nRecvAll != nSendAll*TIMES)
			{
				printf("nRecvAll:%d == nSendAll:(%d)\n", nRecvAll, nSendAll);
				Sleep(1);
				assert(nRecvAll == nSendAll*TIMES);
			}
		}

		
		// 关闭套节字
		::closesocket(s);
	}

	DWORD dwNewTick = GetTickCount();

	printf("\nAll Time:[%8d]\n", dwNewTick - dwTimeTick);
	system("pause");
	return 0;
}









