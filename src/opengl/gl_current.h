#ifndef GL_CURRENT_H
#define GL_CURRENT_H

// Define GLEW_STATIC before including GLEW to indicate we're using the static library
#define GLEW_STATIC

// Prevent Windows from defining min/max/near/far macros
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Define GLEW_NO_GLU to avoid GLU dependencies
#define GLEW_NO_GLU

#include <windows.h>  // For APIENTRY definition on Windows

// Include GLEW before any other OpenGL headers to avoid conflicts
#include "GL/glew.h"  // Include the copied glew header

#endif