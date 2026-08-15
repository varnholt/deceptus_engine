#pragma once
// Nintendo Switch stub — replaces the desktop GLEW header.
//
// GL entry points come from the glad loader that VRSFML bundles inside
// sfml-glutils, not from devkitPro's switch-glad portlib. Both define a gladLoadGL
// symbol, so linking the portlib alongside VRSFML gives a multiple-definition error;
// and since VRSFML loads GL itself when it creates the context, its loader is the one
// that is actually initialised. CMakeLists therefore puts VRSFML's glad include
// directory on the path and does not link switch-glad.
//
// Unlike the WebAssembly stub next door, this exposes *desktop* OpenGL: switch-mesa
// runs on nouveau and reports GL 4.3 core, which is the path the engine's shaders
// target.
//
// How it is activated:
//   CMakeLists.txt prepends src/switch to the include search path before src/ when
//   NINTENDO_SWITCH is defined, so "#include <opengl/glew.h>" resolves here instead
//   of src/opengl/glew.h.

// Guard macros that GLEW's own header sets — keeps the real header from being
// included by any transitively-included file.
#define __glew_h__
#define __GLEW_H__

#include <glad/gl.h>

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY APIENTRY
#endif

// GLEW lifecycle stubs — glewInit is a no-op, because VRSFML has already run its own
// glad loader by the time the engine gets here.
#define GLEW_OK 0

inline int glewInit()
{
   return GLEW_OK;
}

inline const unsigned char* glewGetErrorString(int)
{
   return reinterpret_cast<const unsigned char*>("no error");
}
