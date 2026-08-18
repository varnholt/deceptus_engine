#pragma once
// WebAssembly stub — replaces the desktop GLEW header.
//
// Emscripten provides all WebGL 2 / GLES3 function symbols directly; there is
// no runtime extension-loader step.  This file satisfies the GLEW include
// contract used throughout the engine without pulling in the full 1.2 MB
// GLEW header, which is incompatible with Emscripten's own GL headers.
//
// How it is activated:
//   CMakeLists.txt prepends src/wasm to the include search path before src/
//   when EMSCRIPTEN is defined, so "#include <opengl/glew.h>" resolves here
//   instead of src/opengl/glew.h.

// Guard macros that GLEW's own header sets — keeps the real header from being
// included by any transitively-included file.
#define __glew_h__
#define __GLEW_H__

#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

#ifndef APIENTRY
#define APIENTRY
#endif

// ---------------------------------------------------------------------------
// GLEW lifecycle stubs — glewInit is a no-op; WebGL provides all symbols.
// ---------------------------------------------------------------------------
#define GLEW_OK       0
#define GLEW_NO_ERROR 0
inline int glewInit() { return GLEW_OK; }
inline const char* glewGetErrorString(int /*error_code*/) { return ""; }

// ---------------------------------------------------------------------------
// GL4 shader stage constants absent from GLES3 core.
// Referenced by the GLSLShaderType enum; the unsupported stages are never
// instantiated on WebAssembly.
// ---------------------------------------------------------------------------
#ifndef GL_GEOMETRY_SHADER
#define GL_GEOMETRY_SHADER        0x8DD9
#endif
#ifndef GL_TESS_CONTROL_SHADER
#define GL_TESS_CONTROL_SHADER    0x8E88
#endif
#ifndef GL_TESS_EVALUATION_SHADER
#define GL_TESS_EVALUATION_SHADER 0x8E87
#endif
#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER         0x91B9
#endif

// ---------------------------------------------------------------------------
// GL_KHR_debug suffix-free aliases.
// Emscripten's gl2ext.h defines the _KHR-suffixed variants; add plain names
// if still missing so that GLUtils::debugCallback compiles unmodified.
// ---------------------------------------------------------------------------
#ifndef GL_DEBUG_SOURCE_API
#define GL_DEBUG_SOURCE_API               0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM     0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER   0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY       0x8249
#define GL_DEBUG_SOURCE_APPLICATION       0x824A
#define GL_DEBUG_SOURCE_OTHER             0x824B
#define GL_DEBUG_TYPE_ERROR               0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR  0x824E
#define GL_DEBUG_TYPE_PORTABILITY         0x824F
#define GL_DEBUG_TYPE_PERFORMANCE         0x8250
#define GL_DEBUG_TYPE_MARKER              0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP          0x8269
#define GL_DEBUG_TYPE_POP_GROUP           0x826A
#define GL_DEBUG_TYPE_OTHER               0x8251
#define GL_DEBUG_SEVERITY_HIGH            0x9146
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#define GL_DEBUG_SEVERITY_LOW             0x9148
#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B
#endif

// ---------------------------------------------------------------------------
// GL_ARB_program_interface_query stubs (GL 4.3 / GLES 3.1).
// Only referenced in GLSLProgram::printActive* debug utilities which are
// never called; stubs allow compilation without pulling in gl31.h.
// ---------------------------------------------------------------------------
#ifndef GL_ACTIVE_RESOURCES
#define GL_UNIFORM_BLOCK       0x92E2
#define GL_PROGRAM_INPUT       0x92E3
#define GL_ACTIVE_RESOURCES    0x92F5
#define GL_NUM_ACTIVE_VARIABLES 0x92F7
#define GL_ACTIVE_VARIABLES    0x92F6
#define GL_NAME_LENGTH         0x92F9
#define GL_TYPE                0x92FA
#define GL_LOCATION            0x930E
#define GL_BLOCK_INDEX         0x92FD

inline void glGetProgramInterfaceiv(GLuint /*program*/, GLenum /*programInterface*/, GLenum /*pname*/, GLint* params)
{
    if (params)
    {
        *params = 0;
    }
}
inline void glGetProgramResourceiv(
    GLuint /*program*/,
    GLenum /*programInterface*/,
    GLuint /*index*/,
    GLsizei /*propCount*/,
    const GLenum* /*props*/,
    GLsizei /*bufSize*/,
    GLsizei* length,
    GLint* params
)
{
    if (length)
    {
        *length = 0;
    }
    if (params)
    {
        *params = 0;
    }
}
inline void glGetProgramResourceName(
    GLuint /*program*/,
    GLenum /*programInterface*/,
    GLuint /*index*/,
    GLsizei bufSize,
    GLsizei* length,
    GLchar* name
)
{
    if (length)
    {
        *length = 0;
    }
    if (bufSize > 0 && name)
    {
        name[0] = '\0';
    }
}
#endif

// ---------------------------------------------------------------------------
// glBindFragDataLocation — desktop GL 3.0 function not in GLES3 core.
// Defined in GLSLProgram::bindFragDataLocation which is never called; stub
// prevents an undeclared-identifier error during compilation.
// ---------------------------------------------------------------------------
#ifndef GL_FRAGMENT_SHADER_BIT
inline void glBindFragDataLocation(GLuint /*program*/, GLuint /*colorNumber*/, const char* /*name*/) {}
#endif

// ---------------------------------------------------------------------------
// GL_PROGRAM_POINT_SIZE — GL 3.2 feature absent from GLES3 core.
// ---------------------------------------------------------------------------
#ifndef GL_PROGRAM_POINT_SIZE
#define GL_PROGRAM_POINT_SIZE 0x8642
#endif
