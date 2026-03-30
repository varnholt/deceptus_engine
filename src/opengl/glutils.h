#pragma once

#include "gl_current.h"

/// \brief utility helpers for OpenGL debugging and runtime diagnostics.
namespace GLUtils
{
/// \brief drains OpenGL's error queue and logs each error with source location.
/// \param file source file passed by the caller.
/// \param line source line passed by the caller.
/// \return 1 if at least one error was found, otherwise 0.
int checkForOpenGLError(const char* file, int line);

/// \brief prints vendor, renderer, version and optionally extension strings.
/// \param dumpExtensions when true, prints all available extension names.
void dumpGLInfo(bool dumpExtensions = false);

/// \brief OpenGL debug callback that prints source, type, severity and message.
/// \param source OpenGL-reported message source.
/// \param type OpenGL-reported message type.
/// \param id numeric message identifier from the driver.
/// \param severity OpenGL-reported message severity.
/// \param length message length reported by OpenGL.
/// \param msg null-terminated debug message text.
/// \param param optional user parameter passed by OpenGL.
void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg, const void* param);
}  // namespace GLUtils
