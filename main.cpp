//  This file is part of TinyBeMeViewer by Or Avnat: https://github.com/oravnat/TinyBeMeViewer
//
//  main.cpp
//  TinyBeMeViewer
//
//  Created by Or Avnat on 20/08/2026.
//

#include <iostream>
#include <algorithm>

#define NOMINMAX
#if !__APPLE__
#include "OpenGL.hpp"
#endif
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
 
#if __APPLE__
#include <OpenGL/gl3.h>
#endif

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include "StreamSocket.hpp"
#include "PortableSession.hpp"

typedef float vec2[2];
typedef float vec3[3];

typedef struct Vertex
{
    vec2 pos;
    vec2 tx;
} Vertex;
  
static Vertex vertices[4] =
{
    { { -1.0f, -1.0f }, { 0.0f, 0.0f } },
    { { 1.0f, -1.0f }, { 0.0f, 0.0f } },
    { { -1.0f,  1.0f }, { 0.0f, 0.0f } },
    { { 1.0f,  1.0f }, { 0.0f, 0.0f } }
};

static const char* vertex_shader_text =
"#version 330\n"
"uniform mat4 MVP;\n"
"in vec3 vCol;\n"
"in vec2 vPos;\n"
"out vec3 color;\n"
"void main()\n"
"{\n"
//"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
"    gl_Position = vec4(vPos, 0.0, 1.0);\n"
"    color = vCol;\n"
"}\n";
 
static const char* fragment_shader_text =
"#version 330\n"
"in vec3 color;\n"
"out vec4 fragment;\n"
"uniform sampler2D myTextureSampler;\n"
"void main()\n"
"{\n"
//"    fragment = vec4(color, 1.0);\n"
"      fragment = texture(myTextureSampler, color.xy).rgba; \n"
"}\n";
 

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}
 
#if !defined(WIN32)
#define VK_ESCAPE         0x1B
#define VK_RETURN         0x0D
#define VK_TAB            0x09
#define VK_BACK           0x08

#define VK_LSHIFT         0xA0
#define VK_RSHIFT         0xA1
#define VK_LCONTROL       0xA2
#define VK_RCONTROL       0xA3
#define VK_LMENU          0xA4
#define VK_RMENU          0xA5
#endif

int GlfwKeyToWinVKTable[][2] = {
    { GLFW_KEY_SPACE, 0x20 },
    { GLFW_KEY_APOSTROPHE, 0xDE },
    { GLFW_KEY_COMMA, 0xBC },
    { GLFW_KEY_MINUS, 0xBD },
    { GLFW_KEY_PERIOD, 0xBE },
    { GLFW_KEY_SLASH, 0xBF },
    { GLFW_KEY_SEMICOLON, 0xBA },
    { GLFW_KEY_EQUAL, 0xBB },
    { GLFW_KEY_LEFT_BRACKET, 0xDB },
    { GLFW_KEY_BACKSLASH, 0xDC },
    { GLFW_KEY_RIGHT_BRACKET, 0xDD },
    { GLFW_KEY_GRAVE_ACCENT, 0xC0 },
	{ GLFW_KEY_ESCAPE, VK_ESCAPE },
	{ GLFW_KEY_ENTER, VK_RETURN },
	{ GLFW_KEY_TAB, VK_TAB },
	{ GLFW_KEY_BACKSPACE, VK_BACK },
    { GLFW_KEY_LEFT_SHIFT, VK_LSHIFT },
    { GLFW_KEY_LEFT_CONTROL, VK_LCONTROL },
    { GLFW_KEY_LEFT_ALT, VK_LMENU },
    //{ GLFW_KEY_LEFT_SUPER, VK_LWIN },
    { GLFW_KEY_RIGHT_SHIFT, VK_RSHIFT },
    { GLFW_KEY_RIGHT_CONTROL, VK_RCONTROL },
    { GLFW_KEY_RIGHT_ALT, VK_RMENU },
    //{ GLFW_KEY_RIGHT_SUPER, VK_RWIN }
    { GLFW_KEY_RIGHT, 0x27 },
    { GLFW_KEY_LEFT, 0x25  },
    { GLFW_KEY_DOWN, 0x28 },
    { GLFW_KEY_UP, 0x26 },
    { GLFW_KEY_CAPS_LOCK, 0x14 },
    { GLFW_KEY_F1, 0x70 },
	{ GLFW_KEY_F2, 0x71 },
	{ GLFW_KEY_F3, 0x72 },
	{ GLFW_KEY_F4, 0x73 },
	{ GLFW_KEY_F5, 0x74 },
	{ GLFW_KEY_F6, 0x75 },
	{ GLFW_KEY_F7, 0x76 },
	{ GLFW_KEY_F8, 0x77 },
	{ GLFW_KEY_F9, 0x78 },
	{ GLFW_KEY_F10, 0x79 },
	{ GLFW_KEY_F11, 0x7A },
	{ GLFW_KEY_F12, 0x7B }
};

class GLFWSession : public PortableSession
{
public:
    GLFWSession() : PortableSession(&sData) {}
    void HandlePacket(Packet& packet) override
    {
        if (packet.type == MessageType_DesktopInfo)
            HandleDesktopInfo(packet);
        PortableSession::HandlePacket(packet);
    }
    void CopyToView(Image &img) override
    {
        if (img.bitsPerPixel == 32)
            img.Copy32To32(m_view.data, m_view.widthBytes, m_view.w, m_view.h);
        else
            img.CopyToTransparentIndex(m_view.data, m_view.widthBytes, m_view.w, m_view.h, img.transparentIndex);

        glTexSubImage2D(GL_TEXTURE_2D, 0, m_view.left, m_view.top, m_view.w, m_view.h, GL_RGBA, GL_UNSIGNED_BYTE, m_view.data);
    }
    void HandleDesktopInfo(const Packet& packet)
    {
        if (packet.length < sizeof(DesktopInfo))
            return;

        DesktopInfo desktopInfo;
        memcpy(&desktopInfo, packet.content, sizeof(desktopInfo));
        uint8_t* data = new uint8_t[4 * desktopInfo.monitorWidth * desktopInfo.monitorHeight];
        m_view.Release();
        m_view = Image(data, desktopInfo.monitorWidth, desktopInfo.monitorHeight, 4*desktopInfo.monitorWidth);
        m_bNewView = true;
    }
public:
    TcpSessionData sData;
    Image m_view;
    bool m_bNewView = true;
};

GLFWSession session;

int GlfwKeyToVirtualKey(int key)
{
    if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9'))
		return key; // Return ASCII value for alphanumeric keys
	for (int i = 0; i < sizeof(GlfwKeyToWinVKTable) / sizeof(GlfwKeyToWinVKTable[0]); ++i)
	{
		if (GlfwKeyToWinVKTable[i][0] == key)
			return GlfwKeyToWinVKTable[i][1];
	}
	return -1;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    int vk = GlfwKeyToVirtualKey(key);
    if (vk != -1)
	    session.SendKeyboardEvent(vk, action == GLFW_RELEASE, false);
    //std::cout << (char)key << " - " << std::hex << key << std::endl;
    //std::cout << (char)key << " - " << key << std::endl;
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int x = xpos * session.m_view.w / width;
    int y = ypos * session.m_view.h / height;
	uint32_t zDelta = 0;
	uint32_t additionalFlags = 0;
    if (action == GLFW_PRESS)
    {
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			additionalFlags |= 0x0002; // Left button down
		else if (button == GLFW_MOUSE_BUTTON_RIGHT)
			additionalFlags |= 0x0008; // Right button down
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            additionalFlags |= 0x0020; // Middle button down
    }
    else
    {
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			additionalFlags |= 0x0004; // Left button up
        else if (button == GLFW_MOUSE_BUTTON_RIGHT)
            additionalFlags |= 0x0010; // Right button up
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            additionalFlags |= 0x0040; // Middle button up
    }
    session.SendMouseEvent(x, y, additionalFlags, zDelta);
}
 
static int CheckShaderErrors(GLuint fragment_shader)
{
    GLint compiled = GL_FALSE;
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLogLen = 0;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &infoLogLen);
        if (infoLogLen > 0) {
            GLchar* infoLog = (GLchar*)malloc(infoLogLen);
            if (infoLog) {
                glGetShaderInfoLog(fragment_shader, infoLogLen, NULL, infoLog);
                printf("Could not compile %s shader:\n%s\n",
                    "fragment", infoLog);
                free(infoLog);
            }
        }
        glDeleteShader(fragment_shader);
        return -1;
    }
    return 0;
}

void window_focus_callback(GLFWwindow* window, int focused)
{
    if (focused)
    {
        const char* text = glfwGetClipboardString(NULL);
        if (text)
        {
            session.SendClipboardUTF8(text);
        }
    }
}

int main(int argc, char* argv[])
{
    glfwSetErrorCallback(error_callback);
 
    if (!glfwInit())
        exit(EXIT_FAILURE);
 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
 
    GLFWwindow* window = glfwCreateWindow(640, 480, "Tiny BeMe Viewer", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
 
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetWindowFocusCallback(window, window_focus_callback);
 
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
 
#if !defined(__APPLE__)
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        //fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        return -1;
    }
    std::cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;
#endif
 
    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
 
    const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);
 
    const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);
    
    CheckShaderErrors(fragment_shader);
 
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
 
    //const GLint mvp_location = glGetUniformLocation(program, "MVP");
    const GLint vpos_location = glGetAttribLocation(program, "vPos");
    const GLint vcol_location = glGetAttribLocation(program, "vCol");
 
    GLuint vertex_array;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);
    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) offsetof(Vertex, pos));
    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) offsetof(Vertex, tx));
    
    const int MAX_SIZE = 2048;
    GLuint m_tex;
    glGenTextures(1, &m_tex);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    char* p = new char[MAX_SIZE * MAX_SIZE * 4];
    memset(p, 100, MAX_SIZE * MAX_SIZE * 3);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, MAX_SIZE, MAX_SIZE, 0, GL_BGRA, GL_UNSIGNED_BYTE, p);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MAX_SIZE, MAX_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    int error = glGetError();
    assert(error == GL_NO_ERROR);
    delete[] p;

#if defined(WIN32)
    WSADATA wsd;
    WSAStartup(MAKEWORD(2, 1), &wsd);
#endif

    const char* host = nullptr;
    bool bConnected = false;
    if (argc > 1)
    	host = argv[1];
    if (host)
    {
        std::cout << "Connecting to " << host << std::endl;
        bConnected = session.sData.sock.Connect(host, SCREEN_PORT);
        if (bConnected)
            std::cout << "Succesfully connected." << std::endl;
        else
            std::cout << "Connection failed." << std::endl;
    }
    if (bConnected)
    {
        if (!session.Handshake())
        {
            std::cout << "Handshake failed." << std::endl;
            return -1;
		}
        session.SetRemoteProperty(Property_InitDone, 1);
        session.SetRemoteProperty(Property_AutoUpdateFramesPerSeconds, 40);
        if (!session.NonBlockingMode())
        {
            std::cout << "Failed to set non-blocking mode." << std::endl;
            return -1;
        }
    }
     
    while (!glfwWindowShouldClose(window))
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        //const float ratio = width / (float) height;
 
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
  
        glUseProgram(program);
 		
        if (session.m_bNewView)
        {
            vertices[0].tx[1] = (float)session.m_view.h / MAX_SIZE;
            vertices[1].tx[0] = (float)session.m_view.w / MAX_SIZE;
            vertices[1].tx[1] = (float)session.m_view.h / MAX_SIZE;
            vertices[3].tx[0] = (float)session.m_view.w / MAX_SIZE;
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            session.m_bNewView = false;
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_tex);
        glBindVertexArray(vertex_array);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        int error = glGetError();
        assert(error == GL_NO_ERROR);
 
        glfwSwapBuffers(window);
        glfwPollEvents();
        if (bConnected)
            session.PollEvents();
    }
 
    glfwDestroyWindow(window);
 
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
