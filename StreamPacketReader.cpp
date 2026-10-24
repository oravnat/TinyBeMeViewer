//  This file is part of TinyBeMeViewer by Or Avnat: https://github.com/oravnat/TinyBeMeViewer

#include "StreamPacketReader.hpp"
#include <assert.h>
#include <cstring>

StreamPacketReader::StreamPacketReader(ReadStream* pStream)
{
	m_pStream = pStream;
	m_bytesInBuffer = 0;
	m_bStreamClosed = false;
}

StreamPacketReader::~StreamPacketReader(void)
{
}

int StreamPacketReader::Read(int bytes)
{
	bytes = m_pStream->Read(m_buffer + m_bytesInBuffer, bytes);
	if (bytes == 0)
		m_bStreamClosed = true;
	// Success read?
	if (bytes > 0)
		m_bytesInBuffer += bytes;
	return bytes;
}

int StreamPacketReader::ReadFull(int bytes)
{
	int read = 0;
	do
	{
		int n = Read(bytes - read);
		if (n <= 0)
			return n;
		read += n;
	} while(read < bytes);
	return read;
}

int StreamPacketReader::ReadPacket(uint16_t& type, char* pPacket)
{
	if (m_bytesInBuffer < 4)
	{
		int bytes = ReadFull(4 - m_bytesInBuffer);
		if (bytes <= 0)
			return bytes;
	}
	assert(m_bytesInBuffer >= 4);
	type = m_buffer[0] | ((uint16_t)m_buffer[1] << 8);
    uint16_t len = m_buffer[2] | ((uint16_t)m_buffer[3] << 8);
	if (len > m_bytesInBuffer - 4)
	{
		int bytes = ReadFull(len - (m_bytesInBuffer - 4));
		if (bytes <= 0)
			return bytes;
	}
	assert(len <= m_bytesInBuffer - 4);
	memcpy(pPacket, m_buffer + 4, len);
	if (len == m_bytesInBuffer - 4)
		m_bytesInBuffer = 0;
	else
	{
		memmove(m_buffer, m_buffer + len + 4, m_bytesInBuffer - len - 4);
		m_bytesInBuffer -= (len + 4);
	}
	return len;
}

int StreamPacketReader::ReadPacket(Packet& packet)
{
	// Ensure there are at least 4 bytes in the buffer:
	if (m_bytesInBuffer < 4)
	{
		int bytes = ReadFull(4 - m_bytesInBuffer);
		if (bytes <= 0)
			return bytes;
	}
	assert(m_bytesInBuffer >= 4);

    uint16_t len = m_buffer[2] | ((uint16_t)m_buffer[3] << 8);
	if (len > m_bytesInBuffer - 4)
	{
		int bytes = ReadFull(len - (m_bytesInBuffer - 4));
		if (bytes <= 0)
			return bytes;
	}
	assert(len <= m_bytesInBuffer - 4);
	memcpy(&packet, m_buffer, len + 4);
	if (len == m_bytesInBuffer - 4)
		m_bytesInBuffer = 0;
	else
	{
		memmove(m_buffer, m_buffer + len + 4, m_bytesInBuffer - len - 4);
		m_bytesInBuffer -= (len + 4);
	}
	return len;
}


bool StreamPacketReader::IsStreamClosed()
{
	return m_bStreamClosed;
}

void StreamPacketReader::SetReadStream(ReadStream* pStream)
{
	m_pStream = pStream;
}
