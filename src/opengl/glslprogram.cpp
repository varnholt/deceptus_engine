#include "glslprogram.h"

#include "glutils.h"

#include <sys/stat.h>
#include <fstream>
#include <sstream>

namespace GLSLShaderInfo
{
struct shader_file_extension
{
   const char* ext;
   GLSLShader::GLSLShaderType type;
};

struct shader_file_extension extensions[] = {
   {".vs", GLSLShader::VERTEX},
   {".vert", GLSLShader::VERTEX},
   {".gs", GLSLShader::GEOMETRY},
   {".geom", GLSLShader::GEOMETRY},
   {".tcs", GLSLShader::TESS_CONTROL},
   {".tes", GLSLShader::TESS_EVALUATION},
   {".fs", GLSLShader::FRAGMENT},
   {".frag", GLSLShader::FRAGMENT},
   {".cs", GLSLShader::COMPUTE}
};
}  // namespace GLSLShaderInfo

#ifdef __EMSCRIPTEN__
namespace
{
/// \brief swaps the desktop "#version" directive for the GLSL ES 3.00 one required by WebGL2.
///
/// the "#version" directive must be the literal first line of a shader and cannot be selected
/// via "#ifdef" (the GL_ES macro only becomes visible after the version is parsed), so this one
/// line has to be substituted from C++. every other desktop-vs-ES difference (precision
/// qualifiers, sampler binding layout, ...) is handled inside the shader sources via "#ifdef GL_ES".
std::string prepareShaderSourceForGles(const std::string& source)
{
   std::string body = source;

   // drop a leading "#version ..." line if present
   if (body.rfind("#version", 0) == 0)
   {
      const auto newline_position = body.find('\n');
      if (newline_position != std::string::npos)
      {
         body = body.substr(newline_position + 1);
      }
      else
      {
         body.clear();
      }
   }

   return "#version 300 es\n" + body;
}
}  // namespace
#endif

GLSLProgram::~GLSLProgram()
{
   if (_handle == 0)
   {
      return;
   }

   GLint num_shaders = 0;
   glGetProgramiv(_handle, GL_ATTACHED_SHADERS, &num_shaders);

   auto shader_names = new GLuint[num_shaders];
   glGetAttachedShaders(_handle, num_shaders, NULL, shader_names);

   for (auto shader_index = 0; shader_index < num_shaders; shader_index++)
   {
      glDeleteShader(shader_names[shader_index]);
   }

   glDeleteProgram(_handle);

   delete[] shader_names;
}

void GLSLProgram::compileShader(const char* fileName)
{
   _filename = fileName;

   int num_extensions = sizeof(GLSLShaderInfo::extensions) / sizeof(GLSLShaderInfo::shader_file_extension);

   // Check the file name's extension to determine the shader type
   std::string extension = getExtension(fileName);
   GLSLShader::GLSLShaderType type = GLSLShader::VERTEX;
   bool match_found = false;
   for (int extension_index = 0; extension_index < num_extensions; extension_index++)
   {
      if (extension == GLSLShaderInfo::extensions[extension_index].ext)
      {
         match_found = true;
         type = GLSLShaderInfo::extensions[extension_index].type;
         break;
      }
   }

   // If we didn't find a match, throw an exception
   if (!match_found)
   {
      std::string message = "Unrecognized extension: " + extension;
      throw GLSLProgramException(message);
   }

   // Pass the discovered shader type along
   compileShader(fileName, type);
}

std::string GLSLProgram::getExtension(const char* file_name)
{
   std::string name_string(file_name);

   size_t dot_position = name_string.find_last_of('.');
   if (dot_position != std::string::npos)
   {
      return name_string.substr(dot_position, std::string::npos);
   }
   return "";
}

GLSLProgram& GLSLProgram::operator=(const GLSLProgram&)
{
   return *this;
}

void GLSLProgram::compileShader(const char* fileName, GLSLShader::GLSLShaderType type)
{
   _filename = fileName;

   if (!fileExists(fileName))
   {
      std::string message = std::string("Shader: ") + fileName + " not found.";
      throw GLSLProgramException(message);
   }

   if (_handle <= 0)
   {
      _handle = glCreateProgram();
      if (_handle == 0)
      {
         throw GLSLProgramException("Unable to create shader program.");
      }
   }

   std::ifstream in_file(fileName, std::ios::in);
   if (!in_file)
   {
      std::string message = std::string("Unable to open: ") + fileName;
      throw GLSLProgramException(message);
   }

   // Get file contents
   std::stringstream code;
   code << in_file.rdbuf();
   in_file.close();

   compileShader(code.str(), type, fileName);
}

void GLSLProgram::compileShader(const std::string& source, GLSLShader::GLSLShaderType type, const char* fileName)
{
   _filename = fileName;

   if (_handle <= 0)
   {
      _handle = glCreateProgram();
      if (_handle == 0)
      {
         throw GLSLProgramException("Unable to create shader program.");
      }
   }

   GLuint shader_handle = glCreateShader(type);

#ifdef __EMSCRIPTEN__
   const std::string prepared_source = prepareShaderSourceForGles(source);
   const char* source_c_str = prepared_source.c_str();
#else
   const char* source_c_str = source.c_str();
#endif
   glShaderSource(shader_handle, 1, &source_c_str, nullptr);

   // Compile the shader
   glCompileShader(shader_handle);

   // Check for errors
   int compile_status;
   glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &compile_status);
   if (GL_FALSE == compile_status)
   {
      // Compile failed, get log
      int log_length = 0;
      std::string log_string;
      glGetShaderiv(shader_handle, GL_INFO_LOG_LENGTH, &log_length);
      if (log_length > 0)
      {
         char* log_buffer = new char[log_length];
         int written_count = 0;
         glGetShaderInfoLog(shader_handle, log_length, &written_count, log_buffer);
         log_string = log_buffer;
         delete[] log_buffer;
      }
      std::string message;
      if (fileName)
      {
         message = std::string(fileName) + ": shader compliation failed\n";
      }
      else
      {
         message = "Shader compilation failed.\n";
      }
      message += log_string;

      throw GLSLProgramException(message);
   }
   else
   {
      // Compile succeeded, attach shader
      glAttachShader(_handle, shader_handle);
   }
}

void GLSLProgram::link()
{
   if (_linked)
   {
      return;
   }
   if (_handle <= 0)
   {
      throw GLSLProgramException("Program has not been compiled.");
   }

   glLinkProgram(_handle);

   int link_status = 0;
   glGetProgramiv(_handle, GL_LINK_STATUS, &link_status);
   if (GL_FALSE == link_status)
   {
      // Store log and return false
      int log_length = 0;
      std::string log_string;

      glGetProgramiv(_handle, GL_INFO_LOG_LENGTH, &log_length);

      if (log_length > 0)
      {
         char* log_buffer = new char[log_length];
         int written_count = 0;
         glGetProgramInfoLog(_handle, log_length, &written_count, log_buffer);
         log_string = log_buffer;
         delete[] log_buffer;
      }

      throw GLSLProgramException(std::string("Program link failed:\n") + log_string);
   }
   else
   {
      _uniform_locations.clear();
      _linked = true;
   }
}

void GLSLProgram::use()
{
   if (_handle <= 0 || (!_linked))
   {
      throw GLSLProgramException("Shader has not been linked");
   }
   glUseProgram(_handle);
}

int GLSLProgram::getHandle()
{
   return _handle;
}

bool GLSLProgram::isLinked()
{
   return _linked;
}

void GLSLProgram::bindAttribLocation(GLuint location, const char* name)
{
   glBindAttribLocation(_handle, location, name);
}

void GLSLProgram::bindFragDataLocation(GLuint location, const char* name)
{
   glBindFragDataLocation(_handle, location, name);
}

void GLSLProgram::setUniform(const char* name, float x, float y, float z)
{
   GLint uniform_location = getUniformLocation(name);
   glUniform3f(uniform_location, x, y, z);
}

void GLSLProgram::setUniform(const char* name, const glm::vec3& v)
{
   this->setUniform(name, v.x, v.y, v.z);
}

void GLSLProgram::setUniform(const char* name, const glm::vec4& v)
{
   GLint uniform_location = getUniformLocation(name);
   glUniform4f(uniform_location, v.x, v.y, v.z, v.w);
}

void GLSLProgram::setUniform(const char* name, const glm::vec2& v)
{
   GLint uniform_location = getUniformLocation(name);
   glUniform2f(uniform_location, v.x, v.y);
}

void GLSLProgram::setUniform(const char* name, const glm::mat4& m)
{
   GLint uniform_location = getUniformLocation(name);
   glUniformMatrix4fv(uniform_location, 1, GL_FALSE, &m[0][0]);
}

void GLSLProgram::setUniform(const char* name, const glm::mat3& m)
{
   GLint uniform_location = getUniformLocation(name);
   glUniformMatrix3fv(uniform_location, 1, GL_FALSE, &m[0][0]);
}

void GLSLProgram::setUniform(const char* name, float value)
{
   GLint uniform_location = getUniformLocation(name);
   glUniform1f(uniform_location, value);
}

void GLSLProgram::setUniform(const char* name, int value)
{
   GLint uniform_location = getUniformLocation(name);
   glUniform1i(uniform_location, value);
}

void GLSLProgram::setUniform(const char* name, GLuint value)
{
   GLint uniform_location = getUniformLocation(name);
   glUniform1ui(uniform_location, value);
}

void GLSLProgram::setUniform(const char* name, bool value)
{
   int uniform_location = getUniformLocation(name);
   glUniform1i(uniform_location, value);
}

void GLSLProgram::printActiveUniforms()
{
#if defined(__APPLE__) || defined(__EMSCRIPTEN__)
   // For OpenGL 4.1, use glGetActiveUniform
   GLint num_uniforms;
   GLint uniform_size;
   GLint uniform_location;
   GLint max_name_length;
   GLchar* uniform_name;
   GLsizei written_count;
   GLenum uniform_type;

   glGetProgramiv(_handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_length);
   glGetProgramiv(_handle, GL_ACTIVE_UNIFORMS, &num_uniforms);

   uniform_name = new GLchar[max_name_length];

   printf("Active uniforms:\n");
   printf("------------------------------------------------\n");
   for (int uniform_index = 0; uniform_index < num_uniforms; ++uniform_index)
   {
      glGetActiveUniform(_handle, uniform_index, max_name_length, &written_count, &uniform_size, &uniform_type, uniform_name);
      uniform_location = glGetUniformLocation(_handle, uniform_name);
      printf(" %-5d %s (%s)\n", uniform_location, uniform_name, getTypeString(uniform_type));
   }

   delete[] uniform_name;
#else
   // For OpenGL 4.3 and above, use glGetProgramResource
   GLint num_uniforms = 0;
   glGetProgramInterfaceiv(_handle, GL_UNIFORM, GL_ACTIVE_RESOURCES, &num_uniforms);

   GLenum properties[] = {GL_NAME_LENGTH, GL_TYPE, GL_LOCATION, GL_BLOCK_INDEX};

   printf("%s Active uniforms:\n", _filename.c_str());
   for (int uniform_index = 0; uniform_index < num_uniforms; ++uniform_index)
   {
      GLint results[4];
      glGetProgramResourceiv(_handle, GL_UNIFORM, uniform_index, 4, properties, 4, NULL, results);

      if (results[3] != -1)
      {
         continue;  // Skip uniforms in blocks
      }
      GLint name_buffer_size = results[0] + 1;
      char* uniform_name = new char[name_buffer_size];
      glGetProgramResourceName(_handle, GL_UNIFORM, uniform_index, name_buffer_size, NULL, uniform_name);
      printf("%-5d %s (%s)\n", results[2], uniform_name, getTypeString(results[1]));
      delete[] uniform_name;
   }
#endif
}

void GLSLProgram::printActiveUniformBlocks()
{
#if defined(__APPLE__) || defined(__EMSCRIPTEN__)
   // For OpenGL 4.1, use glGetActiveUniformBlockiv
   GLint written_count;
   GLint max_name_length;
   GLint max_uniform_name_length;
   GLint num_blocks;
   GLint block_binding;
   GLchar* block_name;

   glGetProgramiv(_handle, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &max_name_length);
   glGetProgramiv(_handle, GL_ACTIVE_UNIFORM_BLOCKS, &num_blocks);
   glGetProgramiv(_handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_uniform_name_length);
   GLchar* uniform_name = new GLchar[max_uniform_name_length];
   block_name = new GLchar[max_name_length];

   printf("Active Uniform blocks: \n");
   printf("------------------------------------------------\n");
   for (int block_index = 0; block_index < num_blocks; block_index++)
   {
      glGetActiveUniformBlockName(_handle, block_index, max_name_length, &written_count, block_name);
      glGetActiveUniformBlockiv(_handle, block_index, GL_UNIFORM_BLOCK_BINDING, &block_binding);
      printf("Uniform block \"%s\" (%d):\n", block_name, block_binding);

      GLint num_block_uniforms;
      glGetActiveUniformBlockiv(_handle, block_index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &num_block_uniforms);
      GLint* uniform_indices = new GLint[num_block_uniforms];
      glGetActiveUniformBlockiv(_handle, block_index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, uniform_indices);

      for (int uniform_index = 0; uniform_index < num_block_uniforms; ++uniform_index)
      {
         GLint uniform_array_index = uniform_indices[uniform_index];
         GLint uniform_size;
         GLenum uniform_type;

         glGetActiveUniform(
            _handle, uniform_array_index, max_uniform_name_length, &written_count, &uniform_size, &uniform_type, uniform_name
         );
         printf("    %s (%s)\n", block_name, getTypeString(uniform_type));
      }

      delete[] uniform_indices;
   }
   delete[] block_name;
   delete[] uniform_name;
#else
   GLint num_blocks = 0;

   glGetProgramInterfaceiv(_handle, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &num_blocks);
   GLenum block_properties[] = {GL_NUM_ACTIVE_VARIABLES, GL_NAME_LENGTH};
   GLenum block_active_variables[] = {GL_ACTIVE_VARIABLES};
   GLenum uniform_properties[] = {GL_NAME_LENGTH, GL_TYPE, GL_BLOCK_INDEX};

   for (int block_index = 0; block_index < num_blocks; ++block_index)
   {
      GLint block_resource_info[2];
      glGetProgramResourceiv(_handle, GL_UNIFORM_BLOCK, block_index, 2, block_properties, 2, NULL, block_resource_info);
      GLint num_block_uniforms = block_resource_info[0];

      char* block_name = new char[block_resource_info[1] + 1];
      glGetProgramResourceName(_handle, GL_UNIFORM_BLOCK, block_index, block_resource_info[1] + 1, NULL, block_name);
      printf("Uniform block \"%s\":\n", block_name);
      delete[] block_name;

      GLint* uniform_indices = new GLint[num_block_uniforms];
      glGetProgramResourceiv(_handle, GL_UNIFORM_BLOCK, block_index, 1, block_active_variables, num_block_uniforms, NULL, uniform_indices);

      for (int uniform_index = 0; uniform_index < num_block_uniforms; ++uniform_index)
      {
         GLint uniform_array_index = uniform_indices[uniform_index];
         GLint results[3];
         glGetProgramResourceiv(_handle, GL_UNIFORM, uniform_array_index, 3, uniform_properties, 3, NULL, results);

         GLint name_buffer_size = results[0] + 1;
         char* uniform_name = new char[name_buffer_size];
         glGetProgramResourceName(_handle, GL_UNIFORM, uniform_array_index, name_buffer_size, NULL, uniform_name);
         printf("    %s (%s)\n", uniform_name, getTypeString(results[1]));
         delete[] uniform_name;
      }

      delete[] uniform_indices;
   }
#endif
}

void GLSLProgram::printActiveAttribs()
{
#if defined(__APPLE__) || defined(__EMSCRIPTEN__)
   // For OpenGL 4.1, use glGetActiveAttrib
   GLint written_count;
   GLint attrib_size;
   GLint attrib_location;
   GLint max_name_length;
   GLint num_attribs;
   GLenum attrib_type;
   GLchar* attrib_name;

   glGetProgramiv(_handle, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_name_length);
   glGetProgramiv(_handle, GL_ACTIVE_ATTRIBUTES, &num_attribs);

   attrib_name = new GLchar[max_name_length];
   printf("Active Attributes: \n");
   printf("------------------------------------------------\n");
   for (int attrib_index = 0; attrib_index < num_attribs; attrib_index++)
   {
      glGetActiveAttrib(_handle, attrib_index, max_name_length, &written_count, &attrib_size, &attrib_type, attrib_name);
      attrib_location = glGetAttribLocation(_handle, attrib_name);
      printf(" %-5d %s (%s)\n", attrib_location, attrib_name, getTypeString(attrib_type));
   }
   delete[] attrib_name;
#else
   // >= OpenGL 4.3, use glGetProgramResource
   GLint num_attribs;
   glGetProgramInterfaceiv(_handle, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &num_attribs);

   GLenum properties[] = {GL_NAME_LENGTH, GL_TYPE, GL_LOCATION};

   printf("Active attributes:\n");
   for (int attrib_index = 0; attrib_index < num_attribs; ++attrib_index)
   {
      GLint results[3];
      glGetProgramResourceiv(_handle, GL_PROGRAM_INPUT, attrib_index, 3, properties, 3, NULL, results);

      GLint name_buffer_size = results[0] + 1;
      char* attrib_name = new char[name_buffer_size];
      glGetProgramResourceName(_handle, GL_PROGRAM_INPUT, attrib_index, name_buffer_size, NULL, attrib_name);
      printf("%-5d %s (%s)\n", results[2], attrib_name, getTypeString(results[1]));
      delete[] attrib_name;
   }
#endif
}

const char* GLSLProgram::getTypeString(GLenum type)
{
   // There are many more types than are covered here, but
   // these are the most common in these examples.
   switch (type)
   {
      case GL_FLOAT:
         return "float";
      case GL_FLOAT_VEC2:
         return "vec2";
      case GL_FLOAT_VEC3:
         return "vec3";
      case GL_FLOAT_VEC4:
         return "vec4";
#ifndef __EMSCRIPTEN__
      case GL_DOUBLE:
         return "double";
#endif
      case GL_INT:
         return "int";
      case GL_UNSIGNED_INT:
         return "unsigned int";
      case GL_BOOL:
         return "bool";
      case GL_FLOAT_MAT2:
         return "mat2";
      case GL_FLOAT_MAT3:
         return "mat3";
      case GL_FLOAT_MAT4:
         return "mat4";
      default:
         return "?";
   }
}

void GLSLProgram::validate()
{
   if (!isLinked())
   {
      throw GLSLProgramException("Program is not linked");
   }

   GLint validation_status;
   glValidateProgram(_handle);
   glGetProgramiv(_handle, GL_VALIDATE_STATUS, &validation_status);

   if (GL_FALSE == validation_status)
   {
      // Store log and return false
      int log_length = 0;
      std::string log_string;

      glGetProgramiv(_handle, GL_INFO_LOG_LENGTH, &log_length);

      if (log_length > 0)
      {
         char* log_buffer = new char[log_length];
         int written_count = 0;
         glGetProgramInfoLog(_handle, log_length, &written_count, log_buffer);
         log_string = log_buffer;
         delete[] log_buffer;
      }

      throw GLSLProgramException(std::string("Program failed to validate\n") + log_string);
   }
}

int GLSLProgram::getUniformLocation(const char* name)
{
   auto location_iterator = _uniform_locations.find(name);

   if (location_iterator == _uniform_locations.end())
   {
      _uniform_locations[name] = glGetUniformLocation(_handle, name);
   }

   return _uniform_locations[name];
}

bool GLSLProgram::fileExists(const std::string& fileName)
{
   struct stat file_info;
   int stat_result = -1;

   stat_result = stat(fileName.c_str(), &file_info);
   return 0 == stat_result;
}
