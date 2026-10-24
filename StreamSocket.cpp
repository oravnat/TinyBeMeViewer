//  This file is part of TinyBeMeViewer by Or Avnat: https://github.com/oravnat/TinyBeMeViewer

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX

#include "StreamSocket.hpp"
#include "ReadStream.hpp"

#include <algorithm>
#include <cerrno>

StreamSocket::StreamSocket()
{
	s = INVALID_SOCKET;
}

StreamSocket::~StreamSocket()
{
	if (s != INVALID_SOCKET)
		Close();
}

bool StreamSocket::Connect(const char* host, int port, int localPort)
{
	s = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr_in sockAddr;
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons((u_short)port);
	sockAddr.sin_addr.s_addr = inet_addr(host);
	
	if (sockAddr.sin_addr.s_addr == INADDR_NONE)
    	return false;
	if (localPort != 0)
	{
		struct sockaddr_in sockAddr;
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_port = htons((u_short)localPort);
		sockAddr.sin_addr.s_addr = INADDR_ANY;
		if (bind(s, (struct sockaddr*)&sockAddr, sizeof(sockaddr)) != 0)
			return false;
	}
	if (connect(s, (struct sockaddr*)&sockAddr, sizeof(sockaddr)) != 0)
    	return false;

	return true;
}


int StreamSocket::Send(const void *buf, int len)
{
	int result = (int)send(s, (const char *)buf, len, 0);
	return result;
}


// on success, returns the number of bytes read (may be less than len)
// returns 0 on eof
// returns -1 on error
// returns -2 in case the stream is in non-block mode and there are no data
int StreamSocket::Receive(void *buf, int len)
{
	int result;
	
	result = (int)recv(s, (char *)buf, len, 0);
	if (result < 0)
	{
#ifdef WIN32
		int error = WSAGetLastError();
		// if non-blocking socket and no data available, return StreamNoReadyData:
		if (error == WSAEWOULDBLOCK)
            return ReadStream::StreamNoReadyData;
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return ReadStream::StreamNoReadyData;
#endif

		return result;
	}
	return result;
}

// Should not be used with non-blocking sockets
int StreamSocket::ReceiveFull(void *buf, int len)
{
	int read, n;
	for (read = 0, n = 1; read < len && n > 0; read += n)
		n = (int)recv(s, (char *)buf + read, len - read, 0);
	if (n > 0)
		return read; //OK
	return n; //ERROR
}

int StreamSocket::Send(const char *str)
{
	int len = (int)strlen(str);
    return Send(str, len);
}

// The function returns 1 on success:
int StreamSocket::Skip(int bytes)
{
	const int BUF_SIZE = 4096;
	char buf[BUF_SIZE];

	while (bytes > 0)
	{
		int n = Receive(buf, std::min(bytes, BUF_SIZE));
		if (n <= 0)
			return n;
		bytes -= n;
	}
	return 1; //OK

}

void StreamSocket::Close()
{
#ifdef WIN32
	shutdown(s, SD_SEND);
	closesocket(s);
#else
	shutdown(s, SHUT_WR);
	close(s);
#endif
	s = INVALID_SOCKET;
}


int StreamSocket::Select(StreamSocket sockets[], int n, int msTimeout)
{
	int result = SOCKET_ERROR;
	timeval timeout;
	timeval* pTimeout = NULL;
	fd_set read;
	int nfds = 0;

	if (msTimeout != -1)
	{
		timeout.tv_sec = msTimeout / 1000;
		timeout.tv_usec = ((msTimeout % 1000) * 1000);
		pTimeout = &timeout;
	}

	FD_ZERO(&read);
	for (int i = 0; i < n; i++)
	{
		if (sockets[i].s != INVALID_SOCKET)
		{
			FD_SET(sockets[i].s, &read);
			nfds = std::max((int)sockets[i].s, nfds);
		}
	}

	int selectResult = select(nfds + 1, &read, NULL, NULL, pTimeout);
	if (selectResult > 0)
	{
		for (int i = 0; i < n; i++)
		{
			if (FD_ISSET(sockets[i].s, &read))
			{
				result = i;
				break;
			}
		}
	}
	else
		//result = selectResult;
		result = -1;
	return result;
}

#if defined(WIN32)
static bool make_nonblocking(SOCKET sock) {
    u_long mode = 1; // 1 to enable non-blocking, 0 for blocking
    return ioctlsocket(sock, FIONBIO, &mode) == NO_ERROR;
}
#else
static bool make_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}
#endif

bool StreamSocket::SetNonBlocking()
{
    return make_nonblocking(s);
}
