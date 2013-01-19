//HTTPRequest.h

#pragma once
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

enum HTTP_REQUEST_METHOD
{
	HTTP_REQUEST_METHOD_INVALID,
	HTTP_REQUEST_METHOD_OPTIONS,
	HTTP_REQUEST_METHOD_GET,
	HTTP_REQUEST_METHOD_HEAD,
	HTTP_REQUEST_METHOD_POST,
	HTTP_REQUEST_METHOD_PUT,
	HTTP_REQUEST_METHOD_DELETE,
	HTTP_REQUEST_METHOD_TRACE,
	HTTP_REQUEST_METHOD_CONNECT
};

class HTTPRequest
{
private:
	char* m_RequestURI;
	HTTP_REQUEST_METHOD m_RequestMethod;
	HTTPRequest();
public:
	HTTPRequest(char* p_pData, int p_iLength);

	void EvaluateRequestLine(char* p_pData, int p_iLength);
	void HTTPRequest::SetRequestMethod(char* p_pData);
	void HTTPRequest::SetRequestURI(char* p_pData);
};

#endif //HTTP_REQUEST_H