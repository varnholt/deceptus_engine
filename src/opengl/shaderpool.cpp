#include "shaderpool.h"

#include <iostream>

ShaderPool& ShaderPool::getInstance()
{
   static ShaderPool __instance;
   return __instance;
}

void ShaderPool::add(const std::string& key, const std::filesystem::path& vertex_shader, const std::filesystem::path& fragment_shader)
{
   try
   {
      auto shader = std::make_shared<GLSLProgram>();

      if (!vertex_shader.empty())
      {
         shader->compileShader(vertex_shader.string().data());
      }

      if (!fragment_shader.empty())
      {
         shader->compileShader(fragment_shader.string().data());
      }

      shader->link();
      shader->use();
#ifndef __EMSCRIPTEN__
      // glValidateProgram reflects the current GL state; on WebGL2 validating before any
      // texture units / VAOs are bound reports false failures (e.g. samplers still aliasing
      // texture unit 0), and with C++ exceptions disabled on WASM a throw aborts the module.
      shader->validate();
#endif

      _shaders[key] = shader;
   }
   catch (GLSLProgramException& e)
   {
      std::cerr << e.what() << std::endl;
      exit(EXIT_FAILURE);
   }
}

std::shared_ptr<GLSLProgram> ShaderPool::get(const std::string& key)
{
   return _shaders[key];
}
