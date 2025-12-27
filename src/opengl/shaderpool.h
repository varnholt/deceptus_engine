#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>

#include "opengl/glslprogram.h"

class ShaderPool
{
public:
   static ShaderPool& getInstance();
   std::map<std::string, std::shared_ptr<GLSLProgram>> _shaders;

   void add(const std::string& key, const std::filesystem::path& vertex_shader, const std::filesystem::path& fragment_shader);

   std::shared_ptr<GLSLProgram> get(const std::string& key);

private:
   ShaderPool() = default;
};
