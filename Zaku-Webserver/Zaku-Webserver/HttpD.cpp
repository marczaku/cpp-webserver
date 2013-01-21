#include "stdafx.h"
#include "HttpD.h"
#include "TCPSocket.h"

#define BUFSIZE (1024*1024)

DWORD WINAPI ServerWorkerThread(LPVOID CompletionPortID);

void GetDataForRequest(char* DataBuf, DWORD* BufLen, int BufMaxLen);

// Typedef definition
typedef struct
{
	OVERLAPPED Overlapped;
	WSABUF DataBuf;
	CHAR Buffer[BUFSIZE];
	DWORD BytesSEND;
	DWORD BytesRECV;
} PER_IO_OPERATION_DATA, * LPPER_IO_OPERATION_DATA;

// Structure definition
typedef struct
{
	TCPSocket Socket;
} PER_HANDLE_DATA, * LPPER_HANDLE_DATA;

HttpD::HttpD(IPAddr4 p_ServerAddress)
{
	m_MainThreadHandle=0;
	m_IsRunning=false;
	m_ServerAddress=p_ServerAddress;
	m_ContentPath=0;
	Init("content/");
}

HttpD::HttpD(IPAddr4 p_ServerAddress, const char* p_relativeContentPath)
{
	m_MainThreadHandle=0;
	m_IsRunning=false;
	m_ServerAddress=p_ServerAddress;
	m_ContentPath=0;
	Init(p_relativeContentPath);
}

HttpD::~HttpD()
{
	if(m_ContentPath!=0)
	{
		delete m_ContentPath;
		m_ContentPath=0;
	}
}

void HttpD::Start()
{
	if(m_IsRunning)
	{
		printf("HttpD is already running in %s\r\n",m_ContentPath);
	}
	else
	{
		printf("\r\nHttpD Starting in path %s\r\n", m_ContentPath);
		for(int i = 0; i < m_iNumberOfThreads; i++)
		{
			HANDLE ThreadHandle;

			// Create a server worker thread, and pass the
			// completion port to the thread. NOTE: the
			// ServerWorkerThread procedure is not defined in this listing.
			if((ThreadHandle = CreateThread(NULL, 0, ServerWorkerThread, m_hCompletionPort, 0, NULL)) == NULL)
			{
				printf("\r\nCreateThread() failed with error %d\r\n", GetLastError());
				return;
			}
			// Close the thread handle
			CloseHandle(ThreadHandle);
		}

		m_MainThreadHandle = (HANDLE) _beginthreadex(nullptr, 0, StartMainThread, this, 0, nullptr);
	}
}

void HttpD::Stop()
{

}

void HttpD::Init(const char* p_Path)
{
	if(m_ContentPath!=0)
	{
		delete m_ContentPath;
		m_ContentPath=0;
	}

	if(p_Path[1] == ':' && p_Path[2] == '\\')
	{
		m_ContentPath = strcpy(m_ContentPath,p_Path);
	}
	else
	{
		m_ContentPath=new char[512];
		GetModuleFileNameA(NULL, m_ContentPath, 512);	// returns the absolute path to the executable
		{
			int i=(int)strlen(m_ContentPath)-1;
			while( (i>=0) && (m_ContentPath[i]!='\\') ){i--;}
			i--;
			while( (i>=0) && (m_ContentPath[i]!='\\') ){i--;}
			m_ContentPath[i+1]=0;
			strcat(m_ContentPath,"content");
		}
	}

	printf("\r\nHttpD Initialized.\r\n");

	if((m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0))==NULL)
	{
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		return;
	}

	{
		SYSTEM_INFO SystemInfo;
		GetSystemInfo(&SystemInfo);
		m_iNumberOfThreads=SystemInfo.dwNumberOfProcessors*2;
	}
	

	//m_ThreadPool = new ThreadPool(Config::FixedThreadPoolSize);
}

unsigned int HttpD::StartMainThread( void* p_HttpDServer )
{
	LPPER_IO_OPERATION_DATA PerIoData;
	LPPER_HANDLE_DATA PerHandleData;
	DWORD Flags;
	DWORD RecvBytes;
	HttpD* HttpDServer = (HttpD*)p_HttpDServer;
	TCPAcceptSocket xAccept;
	if(!xAccept.Open(HttpDServer->m_ServerAddress))
	{
		printf("\r\nHttpD Error opening Socket. Shutting down.\r\n");
		xAccept.Close();
		return 1;
	}

	printf("\r\nHttpD Listening socket open.\r\n");


	while(TRUE)
	{
		if(xAccept.ConnectionPending(INFINITE))
		{
			if ((PerHandleData = (LPPER_HANDLE_DATA) GlobalAlloc(GPTR, sizeof(PER_HANDLE_DATA))) == NULL)
				printf("GlobalAlloc() failed with error %d\n", GetLastError());
			int iTimeout=0;
			while(iTimeout<1000) // No data received
			{
				if(xAccept.Accept(PerHandleData->Socket))
				{

					if(CreateIoCompletionPort((HANDLE) PerHandleData->Socket.GetSocketHandle(), HttpDServer->m_hCompletionPort, (ULONG_PTR) PerHandleData, 0)==NULL)
					{
						printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
						return 1;
					}

					// Create per I/O socket information structure to associate with the WSARecv call below
					if ((PerIoData = (LPPER_IO_OPERATION_DATA) GlobalAlloc(GPTR, sizeof(PER_IO_OPERATION_DATA))) == NULL)
					{
						printf("GlobalAlloc() failed with error %d\n", GetLastError());
						return 1;
					}

					ZeroMemory(&(PerIoData->Overlapped), sizeof(OVERLAPPED));
					PerIoData->BytesSEND = 0;
					PerIoData->BytesRECV = 0;
					PerIoData->DataBuf.len = BUFSIZE;
					PerIoData->DataBuf.buf = PerIoData->Buffer;

					printf("Connection started %d.%d.%d.%d:%d on Socket %d\r\n",
						PerHandleData->Socket.GetRemoteAddr().sin_addr.S_un.S_un_b.s_b1,
						PerHandleData->Socket.GetRemoteAddr().sin_addr.S_un.S_un_b.s_b2,
						PerHandleData->Socket.GetRemoteAddr().sin_addr.S_un.S_un_b.s_b3,
						PerHandleData->Socket.GetRemoteAddr().sin_addr.S_un.S_un_b.s_b4,
						PerHandleData->Socket.GetRemoteAddr().GetPort(),PerHandleData->Socket.GetSocketHandle());

					Flags = 0;
					
					if (WSARecv(PerHandleData->Socket.GetSocketHandle(), &(PerIoData->DataBuf), 1, &RecvBytes, &Flags, &(PerIoData->Overlapped), NULL) == SOCKET_ERROR)
					{
						if (WSAGetLastError() != ERROR_IO_PENDING)
						{
							printf("WSARecv() failed with error %d\n", WSAGetLastError());
							return 1;
						}
					}
					iTimeout=1000;
					//_beginthread(ConnectionThread,0,(void*)pxConnect);
				}
				else
				{
					Sleep(5);
					iTimeout++;
					//if(iTimeout>=1000)
					//	delete pxConnect;
				}
			}
		}
	};

	xAccept.Close();

	return 0;
}

unsigned int HttpD::StartConnectionThread( void* HttpDServer )
{
	return 0;
}

DWORD WINAPI ServerWorkerThread(LPVOID CompletionPortID)
{
	HANDLE CompletionPort = (HANDLE) CompletionPortID;
	DWORD BytesTransferred;
	LPPER_HANDLE_DATA PerHandleData;
	LPPER_IO_OPERATION_DATA PerIoData;
	DWORD SendBytes, RecvBytes;
	DWORD Flags=0;

	bool bReqComplete;

	while(TRUE)
	{
		if (GetQueuedCompletionStatus(CompletionPort, &BytesTransferred,
			(PULONG_PTR)&PerHandleData, (LPOVERLAPPED *) &PerIoData, INFINITE) == 0)
		{
			printf("\r\nGetQueuedCompletionStatus(%i) failed with error %d\r\n", CompletionPort, GetLastError());
		}

		// First check to see if an error has occurred on the socket and if so
		// then close the socket and cleanup the SOCKET_INFORMATION structure
		// associated with the socket
		if (BytesTransferred == 0)
		{
			printf("Closing socket %d\n", PerHandleData->Socket.GetSocketHandle());
			PerHandleData->Socket.Close();

			GlobalFree(PerHandleData);
			GlobalFree(PerIoData);
			continue;
		}

		printf("Found Socket:%i\r\n",PerHandleData->Socket);
		if(PerIoData->BytesSEND==0) // No bytes to send? Then we're grabbing a request
		{
			bReqComplete=false;
			if(BytesTransferred)
			{
				PerIoData->BytesRECV+=BytesTransferred;
				PerIoData->DataBuf.buf+=BytesTransferred;
				PerIoData->DataBuf.len-=BytesTransferred;
				if(PerIoData->DataBuf.len-1<=0)
				{
					bReqComplete=true;
				}
				else
				{
					if((PerIoData->DataBuf.buf[-4]=='\r')&&(PerIoData->DataBuf.buf[-3]=='\n') &&
						(PerIoData->DataBuf.buf[-2]=='\r')&&(PerIoData->DataBuf.buf[-1]=='\n'))
					{
						bReqComplete=true;
					}
				}
			}
			if(bReqComplete)
			{
				PerIoData->DataBuf.buf[0]=0;
				printf("%s\r\n",PerIoData->Buffer);
				// Check request and start sending
				GetDataForRequest(PerIoData->Buffer,&(PerIoData->BytesRECV),BUFSIZE);
				PerIoData->DataBuf.buf=PerIoData->Buffer;
				PerIoData->DataBuf.len=PerIoData->BytesRECV;
				PerIoData->BytesSEND=PerIoData->DataBuf.len;
				PerIoData->BytesRECV=0;
				WSASend(PerHandleData->Socket.GetSocketHandle(), &(PerIoData->DataBuf), 1, &RecvBytes, Flags, &(PerIoData->Overlapped), NULL);
			}
			else
			{
				// Continue receiving data until the request is complete
				WSARecv(PerHandleData->Socket.GetSocketHandle(), &(PerIoData->DataBuf), 1, &RecvBytes, &Flags, &(PerIoData->Overlapped), NULL);
			}
		}
		else
		{
			// Continue sending data until the buffer is empty
			PerIoData->BytesSEND-=BytesTransferred;
			PerIoData->DataBuf.buf+=BytesTransferred;
			PerIoData->DataBuf.len-=BytesTransferred;
			if(PerIoData->BytesSEND<=0)
			{
				PerIoData->BytesSEND=0;
				PerIoData->BytesRECV=0;
				PerIoData->DataBuf.buf=PerIoData->Buffer;
				PerIoData->DataBuf.len=BUFSIZE;
				WSARecv(PerHandleData->Socket.GetSocketHandle(), &(PerIoData->DataBuf), 1, &RecvBytes, &Flags, &(PerIoData->Overlapped), NULL);
			}
		}
	}

	printf("\r\nHttpD Shutting down Worker Thread. \r\n");
}

void GetDataForRequest(char* DataBuf, DWORD* BufLen, int BufMaxLen)
{
	char* sBuf=DataBuf;
	if((*BufLen>0))
	{
		sBuf[*BufLen]=0;
		printf("%s\r\n",sBuf);

		int iFileSize=-1;
		int iEndPos=0;

		{
			int i=0;																		//Example for a HTTP-Request:
			char* sUrl=NULL;																//
			// read the requested file's name												// GET /main.css HTTP/1.1
			while((i<*BufLen)&&(sBuf[i]!='\r'))												// Host: localhost					
			{																				// Connection: keep-alive
				if(sBuf[i]==' ')															// [...]
				{
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
			strcpy(sFileName,"C:\\Users\\Marc\\Documents\\Visual Studio 2012\\Projects\\zaku-webserver\\Zaku-Webserver\\content");
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
				if(iFileSize+100>BufMaxLen){iFileSize=BufMaxLen-100;}
				printf("200:'%s' (%d)\r\n",sUrl,iFileSize);
				sprintf_s(sBuf,BufMaxLen,"HTTP/1.0 200 OK\r\n"
					"Content-Length:%d\r\n\r\n",iFileSize);
				int iHeaderLen=(int)strlen(sBuf);
				fread(sBuf+iHeaderLen,iFileSize,1,pFile);
				*BufLen=iHeaderLen+iFileSize;
				fclose(pFile);
			}
		}
	}
}