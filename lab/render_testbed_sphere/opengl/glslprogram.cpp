#include "glslprogram.h"

#include "glutils.h"

#include <fstream>
using std::ifstream;
using std::ios;

#include <sstream>
#include <sys/stat.h>

namespace GLSLShaderInfo {
  struct shader_file_extension {
    const char *ext;
    GLSLShader::GLSLShaderType type;
  };

  struct shader_file_extension extensions[] =
  {
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
}


GLSLProgram::~GLSLProgram()
{
  if (mHandle == 0)
  {
     return;
  }

  GLint numShaders = 0;
  glGetProgramiv(mHandle, GL_ATTACHED_SHADERS, &numShaders);

  auto shaderNames = new GLuint[numShaders];
  glGetAttachedShaders(mHandle, numShaders, NULL, shaderNames);

  for (auto i = 0; i < numShaders; i++)
    glDeleteShader(shaderNames[i]);

  glDeleteProgram(mHandle);

  delete[] shaderNames;
}

void GLSLProgram::compileShader( const char * fileName )
{
   _filename = fileName;

    int numExts = sizeof(GLSLShaderInfo::extensions) / sizeof(GLSLShaderInfo::shader_file_extension);

    // Check the file name's extension to determine the shader type
    string ext = getExtension( fileName );
    GLSLShader::GLSLShaderType type = GLSLShader::VERTEX;
    bool matchFound = false;
    for( int i = 0; i < numExts; i++ ) {
      if( ext == GLSLShaderInfo::extensions[i].ext ) {
        matchFound = true;
        type = GLSLShaderInfo::extensions[i].type;
        break;
      }
    }

    // If we didn't find a match, throw an exception
    if( !matchFound ) {
      string msg = "Unrecognized extension: " + ext;
      throw GLSLProgramException(msg);
    }

    // Pass the discovered shader type along
    compileShader( fileName, type );
  }

string GLSLProgram::getExtension( const char * name ) {
  string nameStr(name);

  size_t loc = nameStr.find_last_of('.');
  if( loc != string::npos ) {
    return nameStr.substr(loc, string::npos);
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

  if( ! fileExists(fileName) )
  {
    string message = string("Shader: ") + fileName + " not found.";
    throw GLSLProgramException(message);
  }

  if( mHandle <= 0 )
  {
    mHandle = glCreateProgram();
    if( mHandle == 0) {
      throw GLSLProgramException("Unable to create shader program.");
    }
  }

  ifstream inFile( fileName, ios::in );
  if( !inFile ) {
    string message = string("Unable to open: ") + fileName;
    throw GLSLProgramException(message);
  }

  // Get file contents
  std::stringstream code;
  code << inFile.rdbuf();
  inFile.close();

  compileShader(code.str(), type, fileName);
}

void GLSLProgram::compileShader(
   const string & source,
   GLSLShader::GLSLShaderType type,
   const char * fileName
)
{
   _filename = fileName;

  if( mHandle <= 0 )
  {
    mHandle = glCreateProgram();
    if( mHandle == 0)
    {
      throw GLSLProgramException("Unable to create shader program.");
    }
  }

  GLuint shaderHandle = glCreateShader(type);

  const char * c_code = source.c_str();
  glShaderSource( shaderHandle, 1, &c_code, nullptr );

  // Compile the shader
  glCompileShader(shaderHandle);

  // Check for errors
  int result;
  glGetShaderiv( shaderHandle, GL_COMPILE_STATUS, &result );
  if( GL_FALSE == result ) {
    // Compile failed, get log
    int length = 0;
    string logString;
    glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &length );
    if( length > 0 ) {
      char * c_log = new char[length];
      int written = 0;
      glGetShaderInfoLog(shaderHandle, length, &written, c_log);
      logString = c_log;
      delete [] c_log;
    }
    string msg;
    if( fileName ) {
      msg = string(fileName) + ": shader compliation failed\n";
    } else {
      msg = "Shader compilation failed.\n";
    }
    msg += logString;

    throw GLSLProgramException(msg);

  } else {
    // Compile succeeded, attach shader
    glAttachShader(mHandle, shaderHandle);
  }
}

void GLSLProgram::link()
{
  if( mLinked ) return;
  if( mHandle <= 0 )
    throw GLSLProgramException("Program has not been compiled.");

  glLinkProgram(mHandle);

  int status = 0;
  glGetProgramiv( mHandle, GL_LINK_STATUS, &status);
  if( GL_FALSE == status ) {
    // Store log and return false
    int length = 0;
    string logString;

    glGetProgramiv(mHandle, GL_INFO_LOG_LENGTH, &length );

    if( length > 0 ) {
      char * c_log = new char[length];
      int written = 0;
      glGetProgramInfoLog(mHandle, length, &written, c_log);
      logString = c_log;
      delete [] c_log;
    }

    throw GLSLProgramException(string("Program link failed:\n") + logString);
  } else {
    uniformLocations.clear();
    mLinked = true;
  }
}

void GLSLProgram::use()
{
  if( mHandle <= 0 || (! mLinked) )
    throw GLSLProgramException("Shader has not been linked");
  glUseProgram( mHandle );
}

int GLSLProgram::getHandle()
{
  return mHandle;
}

bool GLSLProgram::isLinked()
{
  return mLinked;
}

void GLSLProgram::bindAttribLocation( GLuint location, const char * name)
{
  glBindAttribLocation(mHandle, location, name);
}

void GLSLProgram::bindFragDataLocation( GLuint location, const char * name )
{
  glBindFragDataLocation(mHandle, location, name);
}

void GLSLProgram::setUniform( const char *name, float x, float y, float z)
{
  GLint loc = getUniformLocation(name);
  glUniform3f(loc,x,y,z);
}

void GLSLProgram::setUniform( const char *name, const vec3 & v)
{
  this->setUniform(name,v.x,v.y,v.z);
}

void GLSLProgram::setUniform( const char *name, const vec4 & v)
{
  GLint loc = getUniformLocation(name);
  glUniform4f(loc,v.x,v.y,v.z,v.w);
}

void GLSLProgram::setUniform( const char *name, const vec2 & v)
{
  GLint loc = getUniformLocation(name);
  glUniform2f(loc,v.x,v.y);
}

void GLSLProgram::setUniform( const char *name, const mat4 & m)
{
  GLint loc = getUniformLocation(name);
  glUniformMatrix4fv(loc, 1, GL_FALSE, &m[0][0]);
}

void GLSLProgram::setUniform( const char *name, const mat3 & m)
{
  GLint loc = getUniformLocation(name);
  glUniformMatrix3fv(loc, 1, GL_FALSE, &m[0][0]);
}

void GLSLProgram::setUniform( const char *name, float val )
{
  GLint loc = getUniformLocation(name);
  glUniform1f(loc, val);
}

void GLSLProgram::setUniform( const char *name, int val )
{
  GLint loc = getUniformLocation(name);
  glUniform1i(loc, val);
}

void GLSLProgram::setUniform( const char *name, GLuint val )
{
  GLint loc = getUniformLocation(name);
  glUniform1ui(loc, val);
}

void GLSLProgram::setUniform( const char *name, bool val )
{
  int loc = getUniformLocation(name);
  glUniform1i(loc, val);
}

void GLSLProgram::printActiveUniforms() {
#ifdef __APPLE__
  // For OpenGL 4.1, use glGetActiveUniform
  GLint nUniforms, size, location, maxLen;
  GLchar * name;
  GLsizei written;
  GLenum type;

  glGetProgramiv( handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLen);
  glGetProgramiv( handle, GL_ACTIVE_UNIFORMS, &nUniforms);

  name = new GLchar[ maxLen ];

  printf("Active uniforms:\n");
  printf("------------------------------------------------\n");
  for( int i = 0; i < nUniforms; ++i ) {
      glGetActiveUniform( handle, i, maxLen, &written, &size, &type, name );
      location = glGetUniformLocation(handle, name);
      printf(" %-5d %s (%s)\n", location, name, getTypeString(type));
  }

  delete [] name;
#else
  // For OpenGL 4.3 and above, use glGetProgramResource
  GLint numUniforms = 0;
  glGetProgramInterfaceiv( mHandle, GL_UNIFORM, GL_ACTIVE_RESOURCES, &numUniforms);

  GLenum properties[] = {GL_NAME_LENGTH, GL_TYPE, GL_LOCATION, GL_BLOCK_INDEX};

  printf("%s Active uniforms:\n", _filename.c_str());
  for( int i = 0; i < numUniforms; ++i ) {
    GLint results[4];
    glGetProgramResourceiv(mHandle, GL_UNIFORM, i, 4, properties, 4, NULL, results);

    if( results[3] != -1 ) continue;  // Skip uniforms in blocks
    GLint nameBufSize = results[0] + 1;
    char * name = new char[nameBufSize];
    glGetProgramResourceName(mHandle, GL_UNIFORM, i, nameBufSize, NULL, name);
    printf("%-5d %s (%s)\n", results[2], name, getTypeString(results[1]));
    delete [] name;
  }
#endif
}

void GLSLProgram::printActiveUniformBlocks() {
#ifdef __APPLE__
  // For OpenGL 4.1, use glGetActiveUniformBlockiv
  GLint written, maxLength, maxUniLen, nBlocks, binding;
  GLchar * name;

  glGetProgramiv(handle, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &maxLength);
  glGetProgramiv(handle, GL_ACTIVE_UNIFORM_BLOCKS, &nBlocks);
  glGetProgramiv(handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniLen);
  GLchar * uniName = new GLchar[maxUniLen];
  name = new GLchar[maxLength];

  printf("Active Uniform blocks: \n");
  printf("------------------------------------------------\n");
  for( int i = 0; i < nBlocks; i++ ) {
      glGetActiveUniformBlockName( handle, i, maxLength, &written, name );
      glGetActiveUniformBlockiv( handle, i, GL_UNIFORM_BLOCK_BINDING, &binding);
      printf("Uniform block \"%s\" (%d):\n", name, binding);

      GLint nUnis;
      glGetActiveUniformBlockiv( handle, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &nUnis);
      GLint * unifIndexes = new GLint[nUnis];
      glGetActiveUniformBlockiv( handle, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, unifIndexes);

      for( int unif = 0; unif < nUnis; ++unif ) {
        GLint uniIndex = unifIndexes[unif];
        GLint size;
        GLenum type;

        glGetActiveUniform( handle, uniIndex, maxUniLen, &written, &size, &type, uniName );
        printf("    %s (%s)\n", name, getTypeString(type));
      }

      delete [] unifIndexes;
  }
  delete [] name;
  delete [] uniName;
#else
  GLint numBlocks = 0;

  glGetProgramInterfaceiv(mHandle, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &numBlocks);
  GLenum blockProps[] = {GL_NUM_ACTIVE_VARIABLES, GL_NAME_LENGTH};
  GLenum blockIndex[] = {GL_ACTIVE_VARIABLES};
  GLenum props[] = {GL_NAME_LENGTH, GL_TYPE, GL_BLOCK_INDEX};

  for(int block = 0; block < numBlocks; ++block) {
    GLint blockInfo[2];
    glGetProgramResourceiv(mHandle, GL_UNIFORM_BLOCK, block, 2, blockProps, 2, NULL, blockInfo);
    GLint numUnis = blockInfo[0];

    char * blockName = new char[blockInfo[1]+1];
    glGetProgramResourceName(mHandle, GL_UNIFORM_BLOCK, block, blockInfo[1]+1, NULL, blockName);
    printf("Uniform block \"%s\":\n", blockName);
    delete [] blockName;

    GLint * unifIndexes = new GLint[numUnis];
    glGetProgramResourceiv(mHandle, GL_UNIFORM_BLOCK, block, 1, blockIndex, numUnis, NULL, unifIndexes);

    for( int unif = 0; unif < numUnis; ++unif ) {
      GLint uniIndex = unifIndexes[unif];
      GLint results[3];
      glGetProgramResourceiv(mHandle, GL_UNIFORM, uniIndex, 3, props, 3, NULL, results);

      GLint nameBufSize = results[0] + 1;
      char * name = new char[nameBufSize];
      glGetProgramResourceName(mHandle, GL_UNIFORM, uniIndex, nameBufSize, NULL, name);
      printf("    %s (%s)\n", name, getTypeString(results[1]));
      delete [] name;
    }

    delete [] unifIndexes;
  }
#endif
}

void GLSLProgram::printActiveAttribs() {
#ifdef __APPLE__
  // For OpenGL 4.1, use glGetActiveAttrib
  GLint written, size, location, maxLength, nAttribs;
  GLenum type;
  GLchar * name;

  glGetProgramiv(handle, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength);
  glGetProgramiv(handle, GL_ACTIVE_ATTRIBUTES, &nAttribs);

  name = new GLchar[maxLength];
  printf("Active Attributes: \n");
  printf("------------------------------------------------\n");
  for( int i = 0; i < nAttribs; i++ ) {
      glGetActiveAttrib( handle, i, maxLength, &written, &size, &type, name );
      location = glGetAttribLocation(handle, name);
      printf(" %-5d %s (%s)\n", location, name, getTypeString(type));
  }
  delete [] name;
#else
  // >= OpenGL 4.3, use glGetProgramResource
  GLint numAttribs;
  glGetProgramInterfaceiv( mHandle, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &numAttribs);

  GLenum properties[] = {GL_NAME_LENGTH, GL_TYPE, GL_LOCATION};

  printf("Active attributes:\n");
  for( int i = 0; i < numAttribs; ++i ) {
    GLint results[3];
    glGetProgramResourceiv(mHandle, GL_PROGRAM_INPUT, i, 3, properties, 3, NULL, results);

    GLint nameBufSize = results[0] + 1;
    char * name = new char[nameBufSize];
    glGetProgramResourceName(mHandle, GL_PROGRAM_INPUT, i, nameBufSize, NULL, name);
    printf("%-5d %s (%s)\n", results[2], name, getTypeString(results[1]));
    delete [] name;
  }
#endif
}

const char * GLSLProgram::getTypeString( GLenum type ) {
  // There are many more types than are covered here, but
  // these are the most common in these examples.
  switch(type) {
    case GL_FLOAT:
      return "float";
    case GL_FLOAT_VEC2:
      return "vec2";
    case GL_FLOAT_VEC3:
      return "vec3";
    case GL_FLOAT_VEC4:
      return "vec4";
    case GL_DOUBLE:
      return "double";
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
    throw GLSLProgramException("Program is not linked");

  GLint status;
  glValidateProgram( mHandle );
  glGetProgramiv( mHandle, GL_VALIDATE_STATUS, &status );

  if( GL_FALSE == status ) {
    // Store log and return false
    int length = 0;
    string logString;

    glGetProgramiv(mHandle, GL_INFO_LOG_LENGTH, &length );

    if( length > 0 ) {
      char * c_log = new char[length];
      int written = 0;
      glGetProgramInfoLog(mHandle, length, &written, c_log);
      logString = c_log;
      delete [] c_log;
    }

    throw GLSLProgramException(string("Program failed to validate\n") + logString);

  }
}

int GLSLProgram::getUniformLocation(const char * name )
{
  std::map<string, int>::iterator pos;
  pos = uniformLocations.find(name);

  if( pos == uniformLocations.end() ) {
    uniformLocations[name] = glGetUniformLocation(mHandle, name);
  }

  return uniformLocations[name];
}

bool GLSLProgram::fileExists( const string & fileName )
{
  struct stat info;
  int ret = -1;

  ret = stat(fileName.c_str(), &info);
  return 0 == ret;
}
