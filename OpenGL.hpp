//  This file is part of Snake3D by Or Avnat: https://github.com/oravnat/Snake3D_public

#pragma once

#if defined(ANDROID_NDK) || defined(__ANDROID_API__)

// note: using ".. " in the path of include is not recommended and does not work in xcode
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#define glClearDepth glClearDepthf

#else

#if __APPLE__
#include <TargetConditionals.h>
#if TARGET_MACOS || TARGET_OS_OSX
#  include <OpenGL/gl3.h>
#  else // if (TARGET_IOS || TARGET_TVOS)
#  import <OpenGLES/ES3/gl.h>
#  import <OpenGLES/ES3/glext.h>
#  define glClearDepth glClearDepthf
#  endif
#else
#define GLEW_NO_GLU
#include "GL/glew.h"
//#include "GlWrapperWin32.h" // no support for glVertexAttrib functions
#endif

#endif
