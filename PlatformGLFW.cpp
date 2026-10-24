//  This file is part of TinyBeMeViewer by Or Avnat: https://github.com/oravnat/TinyBeMeViewer
//
//  PlatformGLFW.cpp
//  BeMeViewerForMac
//
//  Created by Or Avnat on 05/10/2026.
//

// Implementation using glfw

#include <stdio.h>

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

void SetClipboardString(const char* string) // UTF8
{
    glfwSetClipboardString(NULL, string);
}

const char* GetClipboardString() // UTF8
{
    return glfwGetClipboardString(NULL);
}
