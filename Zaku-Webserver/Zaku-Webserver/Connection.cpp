// Connection.cpp

#include "stdafx.h"
#include "Connection.h"


Connection::Connection()
{
	if ((Buffer = (char*) GlobalAlloc(GPTR, sizeof(char)*1024*1024)) == NULL)
	{
		printf("GlobalAlloc() failed with error %d\n", GetLastError());
	}
	// Create per I/O socket information structure to associate with the WSARecv call below
	if ((OverlappedIO = (OverlappedEx*) GlobalAlloc(GPTR, sizeof(OverlappedEx))) == NULL)
	{
		printf("GlobalAlloc() failed with error %d\n", GetLastError());
	}

	ZeroMemory(OverlappedIO, sizeof(OVERLAPPED));
	OverlappedIO->BytesSEND = 0;
	OverlappedIO->BytesRECV = 0;
	OverlappedIO->DataBuf.len = 1024*1024;
	OverlappedIO->DataBuf.buf = Buffer;
}

Connection::~Connection()
{
	GlobalFree(OverlappedIO);
	delete Buffer;
}