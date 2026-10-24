//  This file is part of TinyBeMeViewer by Or Avnat: https://github.com/oravnat/TinyBeMeViewer

#pragma once

// A class for an IPv4 tcp connection

#ifdef WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define SOCKET int // for linux and mac only!, the windows definition is different!
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

/*
The definition of SOCKET in windows is:
typedef UINT_PTR        SOCKET;
*/
#endif

#include <cstring>
       
class StreamSocket
{
public:
	StreamSocket();
	~StreamSocket();
public:
	bool Connect(const char * host, int port, int localPort = 0);
	int Send(const void *buf, int len);
	int Receive(void *buf, int len);
	int ReceiveFull(void *buf, int len);
	int Send(const char *str);
	int Skip(int bytes);
	void Close();
    bool SetNonBlocking();
	SOCKET GetRawSocket() {return s;}
	void SetRawSocket(SOCKET val) {s = val;}
	static int Select(StreamSocket sockets[], int n, int msTimeout);
protected:
	SOCKET s;
};

