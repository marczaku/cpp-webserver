// HtinyD.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "TCPSocket.h"

//volatile deklarierten Variablen ohne jede Optimierung, d.h. läßt die entsprechenden Werte bei jedem Zugriff neu aus dem Hauptspeicher 
//laden und sorgt bei Veränderungen dafür, daß die neuen Werte ohne Verzögerung dort sofort abgelegt werden.
volatile bool g_bQuit=false;
volatile long g_iThreadCount=0;

char g_sContentPath[512];

void MainThread(void*);
void ConnectionThread(void*);

int _tmain(int argc, _TCHAR* argv[])
{
	/* Initialize WinSock DLL */
	{ // by making use of these braces, the WSADATA-Structure will be removed from the stack after calling the WSAStartup-Method
		WSADATA WSAData;
		WSAStartup(MAKEWORD(2,1), &WSAData); // We could check if the Startup was successful by comparing the return value to zero
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
	printf("Starting in path %s\r\n", g_sContentPath);

	/* Kick off the MainThread that opens a socket and handles incoming requests */
	_beginthread(MainThread,0,NULL);

	/* Main Loop, checking for Quitting conditions */
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


struct ConParams
{
	SOCKET m_xSocket;
	sockaddr_in m_xAddr;
};

/*
	Main Thread

	- registers and binds a new listening socket on port 80
	- runs a loop that checks for new incoming connections on that socket
	- if a new socket for that incoming connection could be created successfully,
	  it runs a new ConnectionThread that handles the connection
*/
void MainThread(void*)
{
	::InterlockedIncrement(&g_iThreadCount);
	IPAddr4 xAddr;
	memset(&xAddr,0,sizeof(xAddr));
	xAddr.Set("localhost:80");

	TCPAcceptSocket xAccept;
	if(!xAccept.Open(xAddr))
	{
		xAccept.Close();
		::InterlockedDecrement(&g_iThreadCount);
		return;
	};

	while(!g_bQuit)
	{
		if(xAccept.ConnectionPending())
		{
			TCPSocket* pxConnect=new TCPSocket();
			int iTimeout=0;
			while(iTimeout<1000) // No data recieved
			{
				if(xAccept.Accept(*pxConnect))
				{
					iTimeout=1000;
					_beginthread(ConnectionThread,0,(void*)pxConnect);
				}
				else
				{
					Sleep(5);
					iTimeout++;
					if(iTimeout>1000)
						delete pxConnect;
				}
			}
		}
	};

	xAccept.Close();
	::InterlockedDecrement(&g_iThreadCount);

};

/*
	Connection Thread

	@param	void* p_pArg	A pointer to a ConParams struct containing the socket reference as well as the other party's address

	Step A: Stream in the request
		- Read data from our socket, until we reached the end of the HTTP-Request, hit the timeout, or an error occured
	Step B: Handle the request
		- Read the requested file out of the HTTP request and try to open it
		- Create a HTTP-Response containing either the file's content or a 404: File not Found Error
		- Stream the Response through our socket until we reach the end or timeout
*/
void ConnectionThread(void* p_pArg)
{
	::InterlockedIncrement(&g_iThreadCount);
	enum {BUFSIZE=1024*1024};
	TCPSocket* pxConSocket=(TCPSocket*)p_pArg;
	printf("Connection thread started %d.%d.%d.%d:%d\r\n",
		pxConSocket->GetRemoteAddr().sin_addr.S_un.S_un_b.s_b1,
		pxConSocket->GetRemoteAddr().sin_addr.S_un.S_un_b.s_b2,
		pxConSocket->GetRemoteAddr().sin_addr.S_un.S_un_b.s_b3,
		pxConSocket->GetRemoteAddr().sin_addr.S_un.S_un_b.s_b4,
		pxConSocket->GetRemoteAddr().GetPort());

	char* sBuf=new char[BUFSIZE];
	bool bReqComplete=false;
	int iBufPos=0;

	int iTimeout=0;

	//*********************************/
	//* Step A: Stream in the request */
	//*********************************/
	while(!g_bQuit&&!bReqComplete)
	{
		if(pxConSocket->WaitRead(5))
		{
			int iS=pxConSocket->Recv(sBuf+iBufPos,BUFSIZE-iBufPos-1);
			if(iS>0) // We recieved data
			{
				/*	- add the recieved data to the buffer
					- if we exceed the buffer-size or reach the end of the request: stop recieving and start evaluating what we got so far */
				iBufPos+=iS;
				if(iBufPos>=BUFSIZE-1)
				{
					bReqComplete=true;
				}
				else
				{
					if((sBuf[iBufPos-4]=='\r')&&(sBuf[iBufPos-3]=='\n') &&
						(sBuf[iBufPos-2]=='\r')&&(sBuf[iBufPos-1]=='\n'))
					{
						bReqComplete=true;
					}
				}
			}
			else if(iS<0 && WSAGetLastError()==WSAEWOULDBLOCK)
			{
				Sleep(5);
				iTimeout++;
				if(iTimeout>1000)
				{
					bReqComplete=true;
					iBufPos=0;
				}
			}
			else
			{
				
				bReqComplete=true;
				iBufPos=0;
			}
		}
		else
		{
			iTimeout++;
			if(iTimeout>1000)
			{
				bReqComplete=true;
				iBufPos=0;
			}
		}
	}

	//*********************************/
	//*  Step B: Handle the request   */
	//*********************************/
	if((iBufPos>0)&&(bReqComplete))
	{
		sBuf[iBufPos]=0;
		printf("%s\r\n",sBuf);

		int iFileSize=-1;
		int iEndPos=0;

		{
			int i=0;																		//Example for a HTTP-Request:
			char* sUrl=NULL;																//
			// read the requested file's name												// GET /main.css HTTP/1.1
			while((i<iBufPos)&&(sBuf[i]!='\r'))												// Host: localhost					
			{																				// Connection: keep-alive
				if(sBuf[i]==' ')															// [...]
				{																			// --- Note: the character after GET and before HTTP are interpreted as '\0'-chars
					if(sUrl==NULL)															
					{																		
						sUrl=sBuf+i+1;														
					}																		
					sBuf[i]=0;																
				}																			
				i++;
			}
			// if a directory was requested, add index.html to the url
			if(strcmp(sUrl,"/")==0)
			{
				strcat(sUrl,"index.html");
			}
			char sFileName[512];
			strcpy(sFileName,g_sContentPath);
			strcat(sFileName,sUrl);
			
			FILE* pFile=NULL;
			pFile=fopen(sFileName,"rb");
			if(!pFile)
			{	// if we could not open the file: put a 404 Error into sBuf
				printf("404:'%s'\r\n",sUrl);
				iFileSize=-1;

				strcpy(sBuf,"HTTP/1.0 404 Not Found\r\n\r\n<html>"
					"<head><title>404</title></head>"
					"<body>Oh noes!</body></html>\r\n\r\n");
				iEndPos=(int)strlen(sBuf);
			}
			else
			{	// put a HTTP-Header as well as the file's content into sBuf
				fseek(pFile,0,SEEK_END);
				iFileSize=ftell(pFile);
				fseek(pFile,0,SEEK_SET);
				if(iFileSize+100>BUFSIZE){iFileSize=BUFSIZE-100;}
				printf("200:'%s' (%d)\r\n",sUrl,iFileSize);
				sprintf_s(sBuf,BUFSIZE,"HTTP/1.0 200 OK\r\n"
					"Content-Length:%d\r\n\r\n",iFileSize);
				int iHeaderLen=(int)strlen(sBuf);
				fread(sBuf+iHeaderLen,iFileSize,1,pFile);
				iEndPos=iHeaderLen+iFileSize;
				fclose(pFile);
			}
		}
		iBufPos=0;
		iTimeout=0;
		/* Send the response via our connection socket until we're finished or have reached a timeout */
		while(!g_bQuit&&(iBufPos<iEndPos))
		{
			if(pxConSocket->CanSendData())
			{
				int iS=pxConSocket->Send(sBuf+iBufPos,iEndPos-iBufPos);
				if(iS<0)
				{
					if(WSAGetLastError()==WSAEWOULDBLOCK)
					{
						Sleep(5);
						iTimeout++;
						if(iTimeout>5000)
						{
							iBufPos=0;
						}
					}
					iBufPos=iEndPos;
				}
				else
					iBufPos+=iS;
			}
			else
			{
				Sleep(5);
				iTimeout++;
				if(iTimeout==1000)
					iBufPos=iEndPos;
			}
		}
	}

	// Shut down the connection and exit this thread
	pxConSocket->Close();
	delete[] sBuf;
	delete pxConSocket;
	::InterlockedDecrement(&g_iThreadCount);
}