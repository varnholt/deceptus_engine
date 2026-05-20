#pragma once

#ifdef WIN32
#pragma warning(disable : 4290)
#endif

#include "gl_current.h"

#include <map>
#include <stdexcept>
#include <string>

#include "glm/glm.hpp"

/// \brief exception type thrown for shader compilation, linking and validation failures.
class GLSLProgramException : public std::runtime_error
{
public:
   /// \brief constructs the exception with a descriptive error message.
   /// \param message message describing the OpenGL program failure.
   GLSLProgramException(const std::string& message) : std::runtime_error(message)
   {
   }
};

namespace GLSLShader
{
/// \brief supported GLSL shader stages mapped to OpenGL shader enums.
enum GLSLShaderType
{
   VERTEX = GL_VERTEX_SHADER,
   FRAGMENT = GL_FRAGMENT_SHADER,
   GEOMETRY = GL_GEOMETRY_SHADER,
   TESS_CONTROL = GL_TESS_CONTROL_SHADER,
   TESS_EVALUATION = GL_TESS_EVALUATION_SHADER,
   COMPUTE = GL_COMPUTE_SHADER
};
};  // namespace GLSLShader

/// \brief manages shader compilation, linking, binding and uniform updates.
class GLSLProgram
{
public:
   GLSLProgram() = default;

   /// \brief deletes attached shader objects and the OpenGL program handle.
   ~GLSLProgram();

   /// \brief compiles a shader file and infers shader stage from file extension.
   /// \param fileName path to the shader source file.
   void compileShader(const char* fileName);

   /// \brief compiles a shader file using an explicit shader stage.
   /// \param fileName path to the shader source file.
   /// \param type shader stage to compile.
   void compileShader(const char* fileName, GLSLShader::GLSLShaderType type);

   /// \brief compiles shader source text and attaches it to this program.
   /// \param source GLSL source code to compile.
   /// \param type shader stage to compile.
   /// \param fileName optional file name used in compiler error messages.
   void compileShader(const std::string& source, GLSLShader::GLSLShaderType type, const char* fileName = nullptr);

   /// \brief links all attached shaders into a usable program.
   void link();

   /// \brief validates the linked program against the current OpenGL state.
   void validate();

   /// \brief binds this program with glUseProgram.
   void use();

   /// \brief returns the raw OpenGL program object handle.
   /// \return non-zero OpenGL program id when created.
   int getHandle();

   /// \brief indicates whether the program has been successfully linked.
   /// \return true when link() has completed successfully.
   bool isLinked();

   /// \brief binds a vertex attribute name to an attribute location before link.
   /// \param location attribute index to bind.
   /// \param name attribute variable name in the shader.
   void bindAttribLocation(GLuint location, const char* name);

   /// \brief binds a fragment output variable to a color attachment location.
   /// \param location fragment output location.
   /// \param name fragment output variable name in the shader.
   void bindFragDataLocation(GLuint location, const char* name);

   /// \brief sets a vec3 uniform from three float components.
   /// \param name uniform variable name.
   /// \param x x-component.
   /// \param y y-component.
   /// \param z z-component.
   void setUniform(const char* name, float x, float y, float z);

   /// \brief sets a vec2 uniform value.
   /// \param name uniform variable name.
   /// \param v vec2 value.
   void setUniform(const char* name, const glm::vec2& v);

   /// \brief sets a vec3 uniform value.
   /// \param name uniform variable name.
   /// \param v vec3 value.
   void setUniform(const char* name, const glm::vec3& v);

   /// \brief sets a vec4 uniform value.
   /// \param name uniform variable name.
   /// \param v vec4 value.
   void setUniform(const char* name, const glm::vec4& v);

   /// \brief sets a mat4 uniform value.
   /// \param name uniform variable name.
   /// \param m matrix data uploaded in column-major order.
   void setUniform(const char* name, const glm::mat4& m);

   /// \brief sets a mat3 uniform value.
   /// \param name uniform variable name.
   /// \param m matrix data uploaded in column-major order.
   void setUniform(const char* name, const glm::mat3& m);

   /// \brief sets a float uniform value.
   /// \param name uniform variable name.
   /// \param value scalar float value.
   void setUniform(const char* name, float value);

   /// \brief sets a signed integer uniform value.
   /// \param name uniform variable name.
   /// \param value scalar integer value.
   void setUniform(const char* name, int value);

   /// \brief sets a boolean uniform value as an integer uniform.
   /// \param name uniform variable name.
   /// \param value boolean value to upload.
   void setUniform(const char* name, bool value);

   /// \brief sets an unsigned integer uniform value.
   /// \param name uniform variable name.
   /// \param value scalar unsigned value.
   void setUniform(const char* name, GLuint value);

   /// \brief prints active non-block uniform names, locations and types.
   void printActiveUniforms();

   /// \brief prints active uniform block names and contained uniforms.
   void printActiveUniformBlocks();

   /// \brief prints active program input attributes and their locations.
   void printActiveAttribs();

   /// \brief maps OpenGL uniform/attribute types to short readable strings.
   /// \param type OpenGL enum type to describe.
   /// \return static string name for known types, or "?" for unknown types.
   const char* getTypeString(GLenum type);

private:
   GLint getUniformLocation(const char* name);
   bool fileExists(const std::string& fileName);
   std::string getExtension(const char* fileName);

   // Make these private in order to make the object non-copyable
   GLSLProgram(const GLSLProgram& /*other*/)
   {
   }
   GLSLProgram& operator=(const GLSLProgram& /*other*/);

   int32_t _handle = 0;
   bool _linked = false;
   std::map<std::string, int> _uniform_locations;
   std::string _filename;
};
