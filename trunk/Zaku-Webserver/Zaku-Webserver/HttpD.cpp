#include "stdafx.h"
#include "HttpD.h"
#include "TCPSocket.h"
#include <io.h>
#include "Connection.h"

#define BUFSIZE (1024*1024)



LPFN_ACCEPTEX lpfnAcceptEx = NULL;
GUID GuidAcceptEx = WSAID_ACCEPTEX;
LPFN_TRANSMITPACKETS lpfnTransmitPackets = NULL;
GUID GuidTransmitPackets = WSAID_TRANSMITPACKETS;


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

	if((m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0))==NULL)
	{
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		return;
	}

	{
		SYSTEM_INFO SystemInfo;
		GetSystemInfo(&SystemInfo);
		m_iNumberOfThreads=SystemInfo.dwNumberOfProcessors;
	}

	printf("HttpD Initialized.\r\n");
}

void HttpD::Start()
{
	if(m_IsRunning)
	{
		printf("HttpD is already running in %s\r\n",m_ContentPath);
	}
	else
	{
		m_IsRunning=true;
		printf("HttpD Starting in path %s\r\n", m_ContentPath);
		m_WorkerThreadHandles = new HANDLE[m_iNumberOfThreads];
		for(int i = 0; i < m_iNumberOfThreads; i++)
		{
			HANDLE ThreadHandle;
			if((ThreadHandle = CreateThread(NULL, 0, ServerWorkerThread, (LPVOID*)this, 0, NULL)) == NULL)
			{
				printf("CreateThread() failed with error %d\r\n", GetLastError());
				return;
			}
			m_WorkerThreadHandles[i]=ThreadHandle;
		}

		m_MainThreadHandle = (HANDLE) _beginthreadex(nullptr, 0, StartMainThread, this, 0, nullptr);
	}
}

void HttpD::Stop()
{
	if(m_IsRunning)
	{
		printf("HttpD shutting down.\r\n");
	}

	for(int i=0;i<m_iNumberOfThreads;i++)
	{
		PostQueuedCompletionStatus(m_hCompletionPort,0,0,0);
	}

	delete[] m_WorkerThreadHandles;
}

unsigned int HttpD::StartMainThread( void* p_HttpDServer )
{
	Connection* NewConnection;
	DWORD Flags;
	DWORD RecvBytes;
	HttpD* HttpDServer = (HttpD*)p_HttpDServer;
	TCPAcceptSocket xAccept;
	if(!xAccept.Open(HttpDServer->m_ServerAddress))
	{
		printf("HttpD Error opening Socket. Shutting down.\r\n");
		xAccept.Close();
		return 1;
	}

	DWORD dwBytes;

	int iResult = WSAIoctl(xAccept.GetSocketHandle(), SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx, sizeof (GuidAcceptEx), 
		&lpfnAcceptEx, sizeof (lpfnAcceptEx), 
		&dwBytes, NULL, NULL);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"WSAIoctl failed with error: %u\n", WSAGetLastError());
		closesocket(xAccept.GetSocketHandle());
		WSACleanup();
		return 1;
	}
	iResult = WSAIoctl(xAccept.GetSocketHandle(), SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidTransmitPackets, sizeof (GuidTransmitPackets), 
		&lpfnTransmitPackets, sizeof (lpfnTransmitPackets), 
		&dwBytes, NULL, NULL);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"WSAIoctl failed with error: %u\n", WSAGetLastError());
		closesocket(xAccept.GetSocketHandle());
		WSACleanup();
		return 1;
	}

	printf("HttpD Listening socket open.\r\n");

	while(TRUE)
	{
		if(xAccept.ConnectionPending(INFINITE))
		{
			NewConnection = new Connection();
			int iTimeout=0;
			while(iTimeout<1000) // No data received
			{
				if(xAccept.Accept(NewConnection->Socket))
				{

					if(CreateIoCompletionPort((HANDLE) NewConnection->Socket.GetSocketHandle(), HttpDServer->m_hCompletionPort, (ULONG_PTR) NewConnection, 0)==NULL)
					{
						printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
						return 1;
					}

					printf("Connection started %d.%d.%d.%d:%d on Socket %d\r\n",
						NewConnection->Socket.GetRemoteAddr().sin_addr.S_un.S_un_b.s_b1,
						NewConnection->Socket.GetRemoteAddr().sin_addr.S_un.S_un_b.s_b2,
						NewConnection->Socket.GetRemoteAddr().sin_addr.S_un.S_un_b.s_b3,
						NewConnection->Socket.GetRemoteAddr().sin_addr.S_un.S_un_b.s_b4,
						NewConnection->Socket.GetRemoteAddr().GetPort(),NewConnection->Socket.GetSocketHandle());

					Flags = 0;

					if (WSARecv(NewConnection->Socket.GetSocketHandle(), &(NewConnection->OverlappedIO->DataBuf), 1, &RecvBytes, &Flags, (LPWSAOVERLAPPED)(NewConnection->OverlappedIO), NULL) == SOCKET_ERROR)
					{
						if (WSAGetLastError() != ERROR_IO_PENDING)
						{
							printf("WSARecv() failed with error %d\n", WSAGetLastError());
						}
					}
					iTimeout=1000;
				}
				else
				{
					Sleep(5);
					iTimeout++;
				}
			}
		}
		else
		{
			break;
		}
	}

	xAccept.Close();

	return 0;
}

DWORD WINAPI HttpD::ServerWorkerThread(LPVOID Server_Handle)
{
	HttpD* Server = (HttpD*) Server_Handle;
	HANDLE CompletionPort = (HANDLE) Server->m_hCompletionPort;
	DWORD BytesTransferred;
	Connection* PerHandleData;
	OverlappedEx* PerIoData;
	DWORD RecvBytes;
	DWORD Flags=0;

	bool bReqComplete;

	while(TRUE)
	{
		if (GetQueuedCompletionStatus(CompletionPort, &BytesTransferred,
			(PULONG_PTR)&PerHandleData, (LPOVERLAPPED *) &PerIoData, INFINITE) == 0)
		{
			printf("GetQueuedCompletionStatus(%i) failed with error %d\r\n", CompletionPort, GetLastError());
		}

		if(PerHandleData==0)
		{
			break;
		}

		// if an error has occurred close the socket and cleanup the SOCKET_INFORMATION structure
		if (BytesTransferred == 0)
		{
			printf("Closing socket %d\n", PerHandleData->Socket.GetSocketHandle());
			PerHandleData->Socket.Close();

			GlobalFree(PerHandleData);
			GlobalFree(PerIoData);
			continue;
		}

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

				TRANSMIT_PACKETS_ELEMENT* PacketElements;

				if ((PacketElements = (TRANSMIT_PACKETS_ELEMENT*) GlobalAlloc(GPTR, sizeof(TRANSMIT_PACKETS_ELEMENT)*2)) == NULL)
					printf("GlobalAlloc() failed with error %d\n", GetLastError());

				GetDataForRequest(PerHandleData->Buffer,&(PerIoData->BytesRECV),BUFSIZE,Server,&(PacketElements[0]),&(PacketElements[1]));
				ZeroMemory(PerIoData, sizeof(OVERLAPPED));

				if(!lpfnTransmitPackets(PerHandleData->Socket.GetSocketHandle(),PacketElements,2,0,(LPOVERLAPPED)(PerIoData),NULL))
				{
					if(WSAGetLastError()!=WSA_IO_PENDING)
						printf("lpfnTransmitPackets(%i) failed with error %d\r\n", CompletionPort, GetLastError());
				}
			}
			else
			{
				// Continue receiving data until the request is complete
				WSARecv(PerHandleData->Socket.GetSocketHandle(), &(PerIoData->DataBuf), 1, &RecvBytes, &Flags, (LPOVERLAPPED)(PerIoData), NULL);
			}
		}
	}

	printf("HttpD Shutting down Worker Thread. \r\n");

	return 0;
}

void HttpD::GetDataForRequest(char* DataBuf, DWORD* BufLen, int BufMaxLen,HttpD* Server, TRANSMIT_PACKETS_ELEMENT* Header, TRANSMIT_PACKETS_ELEMENT* Body)
{
	ZeroMemory((Header), sizeof(TRANSMIT_PACKETS_ELEMENT)*2);

	char* sBuf=DataBuf;
	if((*BufLen>0))
	{
		sBuf[*BufLen]=0;
		printf("%s\r\n",sBuf);

		int iFileSize=-1;
		int iEndPos=0;

		{
			unsigned int i=0;
			char* sUrl=NULL;
			// read the requested file's name
			while((i<*BufLen)&&(sBuf[i]!='\r'))
			{	
				i++;
				if(sBuf[i]==' ')
				{
					if(sUrl==NULL)
					{
						sUrl=sBuf+i+1;
					}
					sBuf[i]=0;
				}
			}
			// if a directory was requested, add index.html to the url
			if(strcmp(sUrl,"/")==0)
			{
				strcat(sUrl,"index.html"); 
			}
			char sFileName[512];
			strcpy(sFileName,Server->m_ContentPath);
			strcat(sFileName,sUrl);

			HANDLE File = CreateFileA(sFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

			if(File==INVALID_HANDLE_VALUE)
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
				{
					LARGE_INTEGER li;
					GetFileSizeEx(File,&li);
					iFileSize=static_cast<int>(li.QuadPart);
				}
				printf("200:'%s' (%dBytes)\r\n",sUrl,iFileSize);
				sprintf_s(sBuf,BufMaxLen,"HTTP/1.0 200 OK\r\n"
					"Content-Length:%d\r\n\r\n",iFileSize);
				int iHeaderLen=(int)strlen(sBuf);
				Header->cLength=iHeaderLen;
				Header->dwElFlags=TP_ELEMENT_MEMORY;
				Header->pBuffer=sBuf;
				Body->dwElFlags=TP_ELEMENT_FILE;
				Body->hFile=File;
			}
		}
	}
}