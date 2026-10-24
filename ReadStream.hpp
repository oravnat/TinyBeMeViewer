//  This file is part of TinyBeMeViewer by Or Avnat: https://github.com/oravnat/TinyBeMeViewer

#pragma once

class ReadStream
{
public:
	virtual int Read(void* pBuf, int len) = 0;
	virtual ~ReadStream() {};
	// StreamClosed is returned if the stream is closed gracefully (the other side
	// has closed the SOCKET for example):
	static const int StreamClosed = 0;
	// StreamError is returned in case of an error (the recv function return -1
	// and the error was not WSAEWOULDBLOCK in the case of SOCKET):
	static const int StreamError = -1;
	// StreamNoReadyData is returned if there are no pending data to read and the 
	// stream is non blocking (WSAEWOULDBLOCK error from from recv for example):
	static const int StreamNoReadyData = -2;
};
