#ifndef IPADDR4_H
#define IPADDR4_H
#include <winsock.h>

class IPAddr4 : public SOCKADDR_IN
{
public:
	IPAddr4();
	IPAddr4(const char* p_sHostAndPort);

	void Dump();
	void SetPort(int p_iPort);
	int GetPort() const;

	void Set(const char* p_sHostAndPort);

	bool operator==(const IPAddr4& p_xA) const
	{
		return (sin_port==p_xA.sin_port) &&
			(sin_addr.S_un.S_addr==p_xA.sin_addr.S_un.S_addr);
	}
};

bool SysErrorToText(int p_iErrorCode, char* po_sText, int p_iLen);

void PrintLastSockError();

class WinsockInitHolder
{
	bool m_bInitialised;
public:
	WinsockInitHolder();
	~WinsockInitHolder();

	bool Initialised() const {return m_bInitialised;}
};

#endif //IPADDR4_H