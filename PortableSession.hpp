//  This file is part of TinyBeMeViewer by Or Avnat: https://github.com/oravnat/TinyBeMeViewer

#include <cassert>
#include <stdint.h>
#include <fcntl.h>

#include "Image.hpp"
#include "StreamPacketReader.hpp"
#include "ProtocolStructures.hpp"


#define MAX_VERSION 2
#define SCREEN_PORT 29539 //115(s)*256 + 99(c)

class SocketReadStream : public ReadStream
{
public:
    SocketReadStream(StreamSocket* pSocket) { m_pSocket = pSocket; }
public:
    virtual int Read(void* pBuf, int len)
    {
        return m_pSocket->Receive(pBuf, len);
    }
//private:
    StreamSocket* m_pSocket;
};


void SetClipboardString(const char* string); // UTF8
const char* GetClipboardString(); // UTF8


struct TcpSessionData
{
    const static int STR_LEN = 512;
    //ScreenStatus status;
    // the session id:
    uint32_t sessionId;
    // The socket connection to the remote user:
    StreamSocket sock;
    // Will be true to signal to stop the current session (but continue to listen on the
    // socket if this is the way the session was initialized):
    volatile bool stopSession;
    // Host to try to connect to in case an invalid socket is supplied:
    char host[STR_LEN];
    // Port to try to connect to or listen to in case an invalid socket is supplied:
    int port;
    bool bNeedWelcome = true;
};

class PortableSession
{
public:
    PortableSession(TcpSessionData* pSessionData) : m_pSessionData(pSessionData), m_pSock(&pSessionData->sock) {}
    int Handshake();
    bool NonBlockingMode() { return m_pSock->SetNonBlocking(); }
    bool PollEvent();
    void PollEvents();
public:
    // Send methods:
    int Send(const void* packet, int len);
    int SendV2Packet(uint16_t type, uint16_t len = 0, const void* pPacket = NULL);
    void SetRemoteProperty(Property prop, uint32_t value);
    void SendKeyboardEvent(int vk, bool up, bool extended);
    void SendMouseEvent(uint32_t x, uint32_t y, uint32_t additionalFlags, uint32_t zDelta);
    void SendClipboardUTF8(const char* text);
public:
    // Packet handlers:
    virtual void HandlePacket(Packet& packet);
    void HandleSetPalette(const Packet& packet);
    void HandlePaletteRect(const Packet& packet);
    void Handle24BitRect(const Packet& packet);
    void HandleScreenInSync(const Packet& packet);
    void HandleCopyUTF8(const Packet& packet);
    virtual void CopyToView(Image &img) = 0;
private:
    // 0 - viewer, 1 - screen overlay:
    int m_activeView = 0;
    TcpSessionData* m_pSessionData;
    StreamSocket* m_pSock;
    SocketReadStream readStream = NULL;
    StreamPacketReader reader = NULL;
    int m_protocolVersion;
    std::string m_clipboardText; // UTF8
    BGRA m_palette[256];
    Image m_img8, m_img32;
};

int PortableSession::Send(const void* packet, int len)
{
    return m_pSock->Send(packet, len);
}

int PortableSession::SendV2Packet(uint16_t type, uint16_t len, const void* pPacket)
{
    char buffer[0x10004];

    buffer[0] = (type & 0xFF);
    buffer[1] = (type >> 8);
    buffer[2] = (len & 0xFF);
    buffer[3] = (len >> 8);
    if (len > 0)
        memcpy(buffer + 4, pPacket, len);
    return Send(buffer, (int)len + 4);
}

// The handshake is symmetric: sending the welcome message, receiving the welcome message,
// sending the session id, receiving the session id, sending the version, receiving
// the version, sending an ok flag,
// receiving an ok flag.
int PortableSession::Handshake()
{
    static const char WELCOME_MESSAGE[] = "v3 TCP connection\n";
    char str[sizeof(WELCOME_MESSAGE)];
    int32_t flag;
    uint32_t version, remoteVersion;
    uint32_t remoteSessionId = 0;

    if (m_pSessionData->bNeedWelcome)
    {
        // Send the welcome message:
        if (m_pSock->Send(WELCOME_MESSAGE, sizeof(WELCOME_MESSAGE)) <= 0)
            return false;
        // Receive the welcome message:
        if (m_pSock->ReceiveFull(&str, sizeof(WELCOME_MESSAGE)) <= 0)
            return false;
        if (strncmp(str, WELCOME_MESSAGE, sizeof(WELCOME_MESSAGE)) != 0)
            return false;
    }
    // Send and receive session id's:
    if (m_pSock->Send(&m_pSessionData->sessionId, sizeof(m_pSessionData->sessionId)) <= 0)
        return false;
    if (m_pSock->ReceiveFull(&remoteSessionId, sizeof(remoteSessionId)) <= 0)
        return false;
    if (remoteSessionId != m_pSessionData->sessionId)
        return false;
    // Send the highest version supported locally:
    version = MAX_VERSION;
    if (m_pSock->Send((char*)&version, sizeof(version)) <= 0)
        return false;
    // Receive the version the remote selected:
    if (m_pSock->ReceiveFull(&remoteVersion, sizeof(remoteVersion)) <= 0)
        return false;
    version = std::min(version, remoteVersion);
    if (version > MAX_VERSION || version <= 1)
    {
        flag = -1; // Unsupported version
        m_pSock->Send(&flag, sizeof(flag));
        return false;
    }
    flag = 1; // Version OK
    if (m_pSock->Send(&flag, sizeof(flag)) <= 0)
        return false;
    if (m_pSock->ReceiveFull(&flag, sizeof(flag)) <= 0)
        return false;
    if (flag != 1)
        return false;
    m_protocolVersion = version;

	readStream.m_pSocket = m_pSock;
    reader.SetReadStream(&readStream);

    return true;
}

void PortableSession::SetRemoteProperty(Property prop, uint32_t value)
{
    Set32BitPropertyInfo info;
    info.key = prop;
    info.value = value;
    SendV2Packet(MessageType_SetLongProperty, sizeof(info), &info);
}

void PortableSession::SendKeyboardEvent(int vk, bool up, bool extended)
{
    KeyEvent content;
    content.vk = (uint8_t)vk;
    content.flags = 0;
    if (up)
        content.flags |= KEYEVENTF_KEYUP;
    if (extended)
        content.flags |= KEYEVENTF_EXTENDEDKEY;

    SendV2Packet(MessageType_Keyboard, sizeof(content), &content);
}

void PortableSession::SendMouseEvent(uint32_t x, uint32_t y, uint32_t additionalFlags, uint32_t zDelta)
{
    MouseEvent pack;
    pack.x = x;
    pack.y = y;
    pack.flags = additionalFlags;
    pack.zDelta = zDelta;
    SendV2Packet(MessageType_MoveMouse, sizeof(pack), &pack);
}

void PortableSession::SendClipboardUTF8(const char* text)
{
    SendV2Packet(MessageType_CopyUTF8, (uint16_t)strlen(text), text);
}


void PortableSession::HandlePacket(Packet& packet)
{
    switch (packet.type)
    {
    case MessageType_NOP:
        break;
    case MessageType_SetPalette:
        HandleSetPalette(packet);
        break;
    case MessageType_DrawPaletteRect:
        HandlePaletteRect(packet);
        break;
    case MessageType_24BitRect:
        Handle24BitRect(packet);
        break;
    //case MessageType_SelfRect:
        //break;
    //case MessageType_DrawOpaqueRect:
        //break;
    //case MessageType_DrawMouseEx:
        //break;
    case MessageType_ScreenInSync:
        HandleScreenInSync(packet);
        break;
    case MessageType_CopyUTF8:
        HandleCopyUTF8(packet);
        break;
    }
}

void PortableSession::HandleSetPalette(const Packet& packet)
{
    if (packet.length <= sizeof(m_palette))
        memcpy(&m_palette, packet.content, packet.length);
}

void PortableSession::HandlePaletteRect(const Packet& packet)
{
    unsigned long count;
    const PaletteRectInfo* pInfo;
    unsigned long compressedLen;
    uint8_t* pData;

    pInfo = (PaletteRectInfo*)&packet.content;
    compressedLen = packet.length - sizeof(PaletteRectInfo);
    int width = pInfo->width;
    int height = pInfo->height;
    // count is the number of expected uncompressed bytes:
    count = width * height;
    
    if (!m_img8.IsEmpty() && m_img8.dataSize >= (int)count)
    {
        pData = m_img8.data;
        memcpy(m_img8.q, m_palette, sizeof(RGBA) * 256);
        m_img8.SetSize(width, height, width);
    }
    else
    {
        pData = new uint8_t[count];
        m_img8.Release();
        m_img8 = Image(pData, width, height, width, m_palette);
    }
    if ((pInfo->flags & UNCOMPRESSED_FLAG) != 0)
    {
        memcpy(pData, (const unsigned char*)packet.content + sizeof(PaletteRectInfo), compressedLen);
        count = compressedLen;
    }
    else
    {
        //uncompress(pData, &count, (const unsigned char*)packet.content + sizeof(PaletteRectInfo), compressedLen);
        // Error: unsupported compressed packet!
        assert(false);
    }

    m_img8.left = pInfo->x;
    m_img8.top = pInfo->y;
    m_img8.transparentIndex = pInfo->transparentIndex;
    m_img8.transparent = (pInfo->transparentIndex != -1);
    
    CopyToView(m_img8);
}

void PortableSession::Handle24BitRect(const Packet& packet)
{
    unsigned long count;
    Bit24RectInfo* pInfo;
    unsigned long compressedLen;

    pInfo = (Bit24RectInfo*)packet.content;
    compressedLen = packet.length - sizeof(Bit24RectInfo);
    int width = pInfo->width;
    int height = pInfo->height;
    // count is the number of expected uncompressed bytes:
    count = width * height * 3;
    
    uint8_t* uncompressed = new uint8_t[count];
    if ((pInfo->flags & UNCOMPRESSED_FLAG) != 0)
    {
        memcpy(uncompressed, (const unsigned char*)packet.content + sizeof(Bit24RectInfo), compressedLen);
        count = compressedLen;
    }
    else
    {
        //uncompress(uncompressed, &count, (const unsigned char*)packet.content + sizeof(Bit24RectInfo), compressedLen);
        // Error: unsupported compressed packet!
        assert(false);
    }

    const uint8_t* pSrc = uncompressed;
    uint8_t* pData = NULL;

    if (!m_img32.IsEmpty() && m_img32.dataSize >= width * height * 4)
    {
        pData = m_img32.data;
        m_img32.SetSize(width, height, width * 4);
    }
    else
    {
        pData = new uint8_t[width * height * 4];
        m_img32.Release();
        m_img32  = Image(pData, width, height, width * 4);
    }

    RGBA* pDest = (RGBA*)(pData);
    int pixelCount = width * height;
    while (pixelCount--)
    {
        pDest->red = pSrc[0];
        pDest->green = pSrc[1];
        pDest->blue = pSrc[2];
        pDest->alpha = 255;
        pSrc += 3;
        pDest++;
    }
    delete[] uncompressed;

    m_img32.left = pInfo->x;
    m_img32.top = pInfo->y;
    // If the message is filtered, unfilter it:
    if ((pInfo->flags & FILTERED_UP_FLAG) != 0)
        m_img32.UnfilterUp();

    CopyToView(m_img32);
}

void PortableSession::HandleScreenInSync(const Packet& packet)
{
}


void PortableSession::HandleCopyUTF8(const Packet& packet)
{
    m_clipboardText.assign((const char*)packet.content, packet.length);
 
    SetClipboardString(m_clipboardText.c_str());
}


bool PortableSession::PollEvent()
{
    Packet packet;
    int nBytes = reader.ReadPacket(packet);
    if (nBytes <= 0)
        return false;
    HandlePacket(packet);
    return true;
}

void PortableSession::PollEvents()
{
    bool bOK = true;
    while (bOK)
        bOK = PollEvent();
}
