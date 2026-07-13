#pragma once

#include <SFML/Graphics.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

//! \brief Wraps sf::Shader so the VRSFML (WASM) vs. vanilla SFML3 (desktop) uniform API divide
//!        lives behind a single call site, mirroring the free-function shims in sfmlcompat.h.
namespace sfcompat
{

//! \brief owns an sf::Shader and presents a single uniform-setting API across both platforms.
//!
//! Vanilla SFML3 (desktop) sets uniforms by string name on every call. VRSFML (WASM) instead
//! requires a cached sf::Shader::UniformLocation obtained once from getUniformLocation() and is
//! not default-constructible (a shader only exists via the loadFromFile() factory). This wrapper
//! hides both differences: setUniform(name, value) reads the same on both platforms, and on
//! VRSFML the location for each name is looked up once and cached so per-frame calls stay cheap.
//! On desktop the call forwards straight to the string-based setUniform with no cache.
//!
//! setUniform() silently ignores a name the loaded shader does not declare, so callers may set
//! optional uniforms unconditionally without first checking whether the shader uses them.
//!
//! A uniform's *name* can still differ per platform: GLSL ES 3.00 (WASM) reserves "texture" as a
//! builtin, so a sampler called "texture" on desktop is usually "u_texture" in the WASM shader
//! source. Such names must still be selected per platform at the call site.
class Shader
{
public:
   Shader() = default;

   /// \brief loads a vertex + fragment shader pair from files.
   /// \return true on success.
   bool loadFromFile(const std::string& vertex_path, const std::string& fragment_path)
   {
#ifdef __EMSCRIPTEN__
      return assign(sf::Shader::loadFromFile({.vertexPath = vertex_path, .fragmentPath = fragment_path}));
#else
      return _shader.loadFromFile(vertex_path, fragment_path) && (_loaded = true);
#endif
   }

   /// \brief loads a fragment-only shader from a file.
   /// \return true on success.
   bool loadFromFragment(const std::string& fragment_path)
   {
#ifdef __EMSCRIPTEN__
      return assign(sf::Shader::loadFromFile({.fragmentPath = fragment_path}));
#else
      return _shader.loadFromFile(fragment_path, sf::Shader::Type::Fragment) && (_loaded = true);
#endif
   }

   /// \brief loads a vertex-only shader from a file.
   /// \return true on success.
   bool loadFromVertex(const std::string& vertex_path)
   {
#ifdef __EMSCRIPTEN__
      return assign(sf::Shader::loadFromFile({.vertexPath = vertex_path}));
#else
      return _shader.loadFromFile(vertex_path, sf::Shader::Type::Vertex) && (_loaded = true);
#endif
   }

   /// \brief returns whether a shader is currently loaded.
   bool isLoaded() const
   {
#ifdef __EMSCRIPTEN__
      return _shader.has_value();
#else
      return _loaded;
#endif
   }

   /// \brief sets a uniform by name; on VRSFML the location is looked up once and cached.
   template <typename Value>
   void setUniform(std::string_view name, const Value& value)
   {
#ifdef __EMSCRIPTEN__
      if (!_shader.has_value())
      {
         return;
      }
      const auto location = uniformLocation(name);
      if (location.has_value())
      {
         (void)_shader->setUniform(*location, value);
      }
#else
      _shader.setUniform(std::string{name}, value);
#endif
   }

   /// \brief returns the underlying sf::Shader for binding into an sf::RenderStates.
   const sf::Shader& native() const
   {
#ifdef __EMSCRIPTEN__
      return *_shader;
#else
      return _shader;
#endif
   }

private:
#ifdef __EMSCRIPTEN__
   //! \brief transparent hasher so the location cache can be probed with a std::string_view key
   //!        without allocating a std::string on every lookup.
   struct StringViewHash
   {
      using is_transparent = void;
      std::size_t operator()(std::string_view text) const noexcept
      {
         return std::hash<std::string_view>{}(text);
      }
   };

   bool assign(sf::base::Optional<sf::Shader>&& loaded)
   {
      if (!loaded.hasValue())
      {
         return false;
      }
      _shader = std::move(*loaded);
      _uniform_locations.clear();
      return true;
   }

   std::optional<sf::Shader::UniformLocation> uniformLocation(std::string_view name)
   {
      const auto cached = _uniform_locations.find(name);
      if (cached != _uniform_locations.end())
      {
         return cached->second;
      }

      const std::string key{name};
      const auto looked_up = _shader->getUniformLocation(key.c_str());
      const std::optional<sf::Shader::UniformLocation> location =
         looked_up.hasValue() ? std::optional<sf::Shader::UniformLocation>{*looked_up} : std::nullopt;
      _uniform_locations.emplace(key, location);
      return location;
   }

   std::optional<sf::Shader> _shader;
   std::unordered_map<std::string, std::optional<sf::Shader::UniformLocation>, StringViewHash, std::equal_to<>> _uniform_locations;
#else
   sf::Shader _shader;
   bool _loaded{false};
#endif
};

}  // namespace sfcompat
