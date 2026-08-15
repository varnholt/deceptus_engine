#pragma once
// Nintendo Switch stub — replaces the desktop GLEW header.
//
// devkitPro's switch-glad already provides a loader generated for OpenGL 4.3 core,
// so there is no GLEW extension-loading step here. This file satisfies the GLEW
// include contract used throughout the engine without pulling in the full 1.2 MB
// GLEW header, which does not agree with mesa's own GL headers.
//
// Unlike the WebAssembly stub next door, this one exposes *desktop* OpenGL rather
// than GLES: switch-mesa runs on nouveau and reports GL 4.3 core, which is the path
// the engine's shaders target.
//
// How it is activated:
//   CMakeLists.txt prepends src/switch to the include search path before src/ when
//   NINTENDO_SWITCH is defined, so "#include <opengl/glew.h>" resolves here instead
//   of src/opengl/glew.h.

// Guard macros that GLEW's own header sets — keeps the real header from being
// included by any transitively-included file.
#define __glew_h__
#define __GLEW_H__

#include <glad/glad.h>

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY APIENTRY
#endif

// The engine calls glewInit() once at start-up. glad needs the same kind of
// one-time load, so the call is mapped onto gladLoadGL() and the GLEW success
// constant is defined to match what the caller checks against.
#ifndef GLEW_OK
#define GLEW_OK 0
#endif

inline unsigned int glewInit()
{
   return gladLoadGL() ? GLEW_OK : 1;
}

inline const unsigned char* glewGetErrorString(unsigned int)
{
   return reinterpret_cast<const unsigned char*>("glad failed to load the OpenGL function pointers");
}
