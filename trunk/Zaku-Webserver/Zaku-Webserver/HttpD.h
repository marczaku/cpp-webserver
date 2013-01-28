// HttpD.h

#pragma once
#ifndef HTTPD_H_INCLUDED
#define HTTPD_H_INCLUDED

#include "IPAddr4.h"
#include "Connection.h"

//class ThreadPool;

using namespace std;

class HttpD
{
public:
	HttpD(IPAddr4 ServerAddress);
	HttpD(IPAddr4 ServerAddress, const char* _relativeContentPath);
	~HttpD();

	void Start();
	void Stop();

 	static unsigned int _stdcall StartMainThread(void* HttpDServer);
	static void GetDataForRequest(char* DataBuf, DWORD* BufLen, int BufMaxLen,HttpD* Server, TRANSMIT_PACKETS_ELEMENT* Header, TRANSMIT_PACKETS_ELEMENT* Body);
	static DWORD WINAPI ServerWorkerThread(LPVOID CompletionPortID);

private:
	int			m_iNumberOfThreads;
	bool		m_IsRunning;
	char*		m_ContentPath; // map (vhosts)
	HANDLE		m_MainThreadHandle;
	HANDLE		m_hCompletionPort;
	IPAddr4		m_ServerAddress;
	HANDLE*		m_WorkerThreadHandles;
	Connection* m_Connections;

private:
	HttpD(const HttpD&);
	HttpD& operator=(const HttpD&);
	void Init(const char* Path);
};

#endif // HTTPD_H_INCLUDED