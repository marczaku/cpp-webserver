// HtinyD.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "TCPSocket.h"
#include "HTTPRequest.h"
#include "HttpD.h"

volatile bool g_bQuit=false;
volatile long g_iThreadCount=0;
char g_sContentPath[512];

int _tmain(int argc, _TCHAR* argv[])
{
	{ /* Initialize Win-Socket */
		WSADATA WSAData;
		WSAStartup(MAKEWORD(2,1), &WSAData);
	}

	/* Initialize the Content-Path (exeDir/../content) */
	GetModuleFileNameA(NULL, g_sContentPath, 512);	// returns the absolute path to the executable
	{
		int i=(int)strlen(g_sContentPath)-1;
		while( (i>=0) && (g_sContentPath[i]!='\\') ){i--;}
		i--;
		while( (i>=0) && (g_sContentPath[i]!='\\') ){i--;}
		g_sContentPath[i+1]=0;
		strcat(g_sContentPath,"content");
	}

	IPAddr4 xAddr;
	memset(&xAddr,0,sizeof(xAddr));
	xAddr.Set("localhost:80");

	HttpD xhttpD(xAddr);
	xhttpD.Start();

	while(!g_bQuit)
	{
		if(kbhit())
		{
			char c = getch();
			if(c == 27)
			{
				g_bQuit = true;
			}
		}
		Sleep(50);
	};

	xhttpD.Stop();

	/* Don't quit until all threads are finished */
	printf("Waiting for threads to finish\r\n");
	while(g_iThreadCount>0)
	{
		Sleep(50);
	}

	printf("Finished\r\n");
	WSACleanup();

	return 0;
}