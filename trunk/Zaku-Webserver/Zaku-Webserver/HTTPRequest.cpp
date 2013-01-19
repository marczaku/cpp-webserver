//HTTPRequest.cpp

#include "stdafx.h"
#include "HTTPRequest.h"

HTTPRequest::HTTPRequest(char* p_pData, int p_iLength)
{
	m_RequestURI=NULL;
	enum {BUFSIZE=1024*1024};
	char* pData=new char[BUFSIZE];
	strcpy(pData,p_pData);
	// We will handle the request line here 
	EvaluateRequestLine(pData,p_iLength);
	//EvaluateRequestHeader(pData,p_iLength);
	delete pData;
}

void HTTPRequest::EvaluateRequestLine(char* p_pData,int p_iLength)
{
	int i=0;
	int iSpaceCount=0;
	int s;
	while(i<p_iLength && p_pData[i]!='\r' && iSpaceCount<2)
	{
		if(p_pData[i]==' ')
		{
			iSpaceCount++;
			p_pData[i]=0;
			if(iSpaceCount==1)
			{
				if(i>0){SetRequestMethod(p_pData);}
				s=i+1;
			}
			else if(iSpaceCount==2)
			{
				if((i-s)>0){SetRequestURI(p_pData+s);}
				s=i+1;
			}
		}
		i++;
	}
}

void HTTPRequest::SetRequestMethod(char* p_pData)
{
	if(strcmp(p_pData,"GET")==0)
	{
		m_RequestMethod = HTTP_REQUEST_METHOD_GET;
	}
	else
	{
		m_RequestMethod = HTTP_REQUEST_METHOD_INVALID;
	}
}

void HTTPRequest::SetRequestURI(char* p_pData)
{
	if(m_RequestURI!=NULL)
		delete m_RequestURI;
	m_RequestURI=new char[512];
	strcpy(m_RequestURI,p_pData);
}