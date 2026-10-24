//  This file is part of TinyBeMeViewer by Or Avnat: https://github.com/oravnat/TinyBeMeViewer

#if __APPLE__
#define PACKED __attribute__((__packed__))
#else
#define PACKED
#endif

#ifndef KEYEVENTF_KEYUP
#define KEYEVENTF_EXTENDEDKEY 0x0001
#define KEYEVENTF_KEYUP       0x0002
#endif

#define FILTERED_UP_FLAG 0x1
#define UNCOMPRESSED_FLAG 0x2

enum MessageType
{
    MessageType_UpdateRequest, MessageType_Mouse_old, MessageType_Keyboard,
    MessageType_ValidProperties, MessageType_OpenNewUpload, MessageType_OpenFile,
    MessageType_OpenNewFile, MessageType_WriteFile, MessageType_CloseFile,
    MessageType_TransferDetails, MessageType_CopyUnicodeText, MessageType_CopyUTF8,
    MessageType_OpenNewSocket, MessageType_WriteZlibFile,
    MessageType_FileInfo, MessageType_NA3, MessageType_SetLongProperty,
    MessageType_SetClipboardFilesWChar, MessageType_NA4, MessageType_MD5Password,
    MessageType_NOP, MessageType_MessageStatus, MessageType_NA5,
    MessageType_ChatString, MessageType_CommandString, MessageType_DesktopInfo,
    MessageType_AccessBits, MessageType_RandomString, MessageType_NA6,
    MessageType_DrawOpaqueRect, MessageType_SetPalette, MessageType_DrawCachedRect,
    MessageType_DrawPaletteRect, MessageType_Cache24BitRect, MessageType_24BitRect,
    MessageType_ScreenInSync, MessageType_MouseMoved, MessageType_DrawMouseEx,
    MessageType_SocketWrite, MessageType_SocketClose, MessageType_SocketStartReceive,
    MessageType_KeyHooked, MessageType_SelfRect, MessageType_ChangeResolution, MessageType_NA7,
    MessageType_NA8, MessageType_NA9,
    MessageType_GetFileInfo, MessageType_GetDirInfo, MessageType_GetFileMD5, MessageType_DirContent,
    MessageType_Username, MessageType_RequestAccess, MessageType_MoveMouse, MessageType_DHPublicKey,
    MessageType_DHStartEncrypt, MessageType_GetFilesInDir,
    NumberOfMessageTypes // must be last in the enum
};

// Supported on macos/tiny version:
//    MessageType_Keyboard(2), MessageType_CopyUTF8(11), MessageType_SetLongProperty(16), MessageType_NOP(20),
//    MessageType_DesktopInfo(25), MessageType_SetPalette(30), MessageType_DrawPaletteRect(32), MessageType_24BitRect(34),
//    MessageType_ScreenInSync(35), MessageType_MoveMouse(53)
// soon be supported on macos: 
//    MessageType_MD5Password(19), MessageType_MessageStatus(21), MessageType_AccessBits(26),
//    MessageType_RandomString(27), MessageType_MouseMoved(36), MessageType_DrawMouseEx(37), MessageType_ChangeResolution(43),
//    MessageType_Username(51), MessageType_RequestAccess(52), 

enum Property
{
    Property_EnableDrawMouseMessages, Property_MonitorToShow, Property_NextMonitor,
    Property_HardwareLockBits, Property_AutoUpdateFramesPerSeconds, Property_ActiveView, Property_RecrodKeys,
    Property_WorkingFolder, Property_OpenFlags, Property_DoNotUseMirror, Property_ResizeWidth, Property_ResizeHeight,
    Property_InitDone, Property_Flags, Property_LowVersion
};

#pragma pack(1)

// MessageType_Keyboard
struct PACKED KeyEvent
{
    uint32_t flags;
    uint8_t vk;
};

// MessageType_SetLongProperty
struct Set32BitPropertyInfo
{
    uint32_t key;
    uint32_t value;
};

// MessageType_DesktopInfo
struct DesktopInfo
{
    int32_t desktopX;
    int32_t desktopY;
    int32_t desktopWidth;
    int32_t desktopHeight;
    int32_t monitorX;
    int32_t monitorY;
    int32_t monitorWidth;
    int32_t monitorHeight;
};

// MessageType_DrawPaletteRect
struct PaletteRectInfo
{
    uint32_t x, y;
    uint32_t width, height;
    int32_t transparentIndex;
    uint32_t flags;
};

// MessageType_24BitRect
struct Bit24RectInfo
{
    uint32_t x, y;
    uint32_t width, height;
    // Bit 0 is on if a filter is used (the filter is the difference between
    // the byte and the byte in the row above):
    uint32_t flags;
};

// MessageType_MoveMouse
struct MouseEvent
{
    uint32_t flags;
    uint32_t x;
    uint32_t y;
    uint32_t zDelta;
};

// Sent from the server to the viewer
// The x and y are of the hot spot:
struct MouseMovedInfo
{
    int32_t x, y;
};

// The mask and color arrays follow this structure.
// sizeof: 26
struct PACKED DrawMouseInfoEx
{
    int32_t left, top; // left and top of the rectangle
    int32_t width, height;
    int32_t xSpot, ySpot;
    uint8_t plans, bitsPerPixel;
};

#pragma pack()
