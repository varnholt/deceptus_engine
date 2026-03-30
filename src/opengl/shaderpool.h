#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>

#include "opengl/glslprogram.h"

/// \brief singleton cache for reusable GLSL programs addressed by string key.
class ShaderPool
{
public:
   /// \brief returns the process-wide shader pool instance.
   /// \return singleton shader pool reference.
   static ShaderPool& getInstance();

   std::map<std::string, std::shared_ptr<GLSLProgram>> _shaders;

   /// \brief creates, links and validates a shader program and stores it under key.
   /// \param key program lookup key.
   /// \param vertex_shader path to vertex shader source file.
   /// \param fragment_shader path to fragment shader source file.
   void add(const std::string& key, const std::filesystem::path& vertex_shader, const std::filesystem::path& fragment_shader);

   /// \brief returns the shader program stored under key.
   /// \param key program lookup key.
   /// \return shared program pointer, possibly null if key is missing.
   std::shared_ptr<GLSLProgram> get(const std::string& key);

private:
   ShaderPool() = default;
};
