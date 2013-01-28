// Connection.h

#pragma once
#include "stdafx.h"
#include "TCPSocket.h"

class OverlappedEx : public OVERLAPPED
{
public:
	TCPSocket*	Socket;
	WSABUF		DataBuf;
	DWORD		BytesRECV,BytesSEND;
};

class Connection
{
public:
	enum ConnectionStatus
	{
		CS_RECV=1,
		CS_ACCEPT=2,
		CS_SEND=CS_RECV & 4
	};

	char* Buffer;
	OverlappedEx*		OverlappedIO;
	ConnectionStatus	ConnectionState;
	TCPSocket			Socket;

	Connection();
	~Connection();
};