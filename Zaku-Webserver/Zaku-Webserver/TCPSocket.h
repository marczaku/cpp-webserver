//TCPSocket.h

#pragma once
#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#ifndef IPADDR4_H
#include "IPAddr4.h"
#endif

class TCPAcceptSocket; // Listening Socket - forward declaration

class TCPSocket
{
	friend class TCPAcceptSocket;
private:
	TCPSocket(const TCPSocket&);
	TCPSocket& operator=(const TCPSocket&);

	SOCKET m_xSock;
	WSAEVENT m_hClose;

public:

	TCPSocket();
	~TCPSocket();

	bool Connect(IPAddr4 p_xTarget);
	void Close();

	bool IsConnected() const;

	int GetPendingReadData() const;
	bool CanSendData() const;
	bool CanRecvData() const;

	bool WaitRead(int p_iTimeout);

	bool HasHalfClose() const;

	int Recv(void* p_pData, int p_iSize, bool p_bPeek=false);
	int Send(const void* p_pData, int p_iSize);

	IPAddr4 GetLocalAddr() const;
	IPAddr4 GetRemoteAddr() const;
};

class TCPAcceptSocket
{
private:
	TCPAcceptSocket(const TCPAcceptSocket&);
	TCPAcceptSocket& operator=(const TCPAcceptSocket&);

	SOCKET m_xSock;
	WSAEVENT m_hEvent;
public:
	TCPAcceptSocket();
	~TCPAcceptSocket();

	bool IsOpen() const;
	bool Open(IPAddr4 p_xAddr, int p_iMaxPending=-1,int p_iMaxRetriesOnUsed=0);
	void Close();

	IPAddr4 GetLocalAddr() const;

	bool ConnectionPending(int p_iTimeout=0);
	bool Accept(TCPSocket& po_rxConnection);
};

#endif // TCPSOCKET_H