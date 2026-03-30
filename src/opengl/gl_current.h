#ifndef GL_CURRENT_H
#define GL_CURRENT_H

/// \brief includes platform and extension loader headers required before OpenGL usage.
///
/// this header ensures windows.h is included on windows builds before GLEW.
#ifdef _WIN32
#include <windows.h>
#endif

#include "opengl/glew.h"

#endif
