#pragma once
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#ifndef GL_STENCIL_INDEX
#define GL_STENCIL_INDEX 0x1901
#endif
#else
#include <SFML/OpenGL.hpp>
#endif
