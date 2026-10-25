//  This file is part of TinyBeMeViewer by Or Avnat: https://github.com/oravnat/TinyBeMeViewer

#pragma once

#include <stdint.h> 

#include "ReadStream.hpp"

/*
Every packet is:
type           (2 bytes)
length         (2 bytes)
packet content (length bytes)
*/

template <int maxSize>
struct PacketT
{
	uint16_t type;
	uint16_t length;
	uint8_t content[maxSize];
};

typedef PacketT<0xFFFF> Packet;

class StreamPacketReader
{
public:
	StreamPacketReader(ReadStream* pStream);
	~StreamPacketReader(void);
public:
	static const int BUFFER_SIZE = 0x10004;
public:
	int ReadPacket(uint16_t& type, char* pPacket);
	int ReadPacket(Packet& packet);
	bool IsStreamClosed();
	void SetReadStream(ReadStream* pStream);
private:
	ReadStream* m_pStream;
	// The buffer always starts at packet boundary.  The buffer contains the type
	// and length of the packet as well.
	uint8_t m_buffer[BUFFER_SIZE];
	int m_bytesInBuffer;
	bool m_bStreamClosed;
private:
	int Read(int bytes);
	int ReadFull(int bytes);
};
