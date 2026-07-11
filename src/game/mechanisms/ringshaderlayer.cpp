#include "ringshaderlayer.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "game/io/valuereader.h"

#include <algorithm>
#ifndef __EMSCRIPTEN__
#include <fstream>
#include <sstream>
#endif

namespace
{
struct RingShaderLayerRegister
{
   RingShaderLayerRegister()
   {
      ShaderLayer::registerCustomization("ring", [](GameNode* parent) { return std::make_shared<RingShaderLayer>(parent); });
   }
};

static RingShaderLayerRegister reg;
}  // namespace

RingShaderLayer::RingShaderLayer(GameNode* parent) : ShaderLayer(parent)
{
}

#ifdef __EMSCRIPTEN__
void RingShaderLayer::checkUniforms()
{
   ShaderLayer::checkUniforms();
   if (!_shader)
   {
      return;
   }
   const auto get_loc = [this](const char* name) -> std::optional<sf::Shader::UniformLocation>
   {
      auto result = _shader->getUniformLocation(name);
      if (result.hasValue())
      {
         return *result;
      }
      return std::nullopt;
   };
   _u_ring_scale_loc = get_loc("u_ring_scale");
   _u_pixel_size_loc = get_loc("u_pixel_size");
   _u_flash_color_loc = get_loc("u_flash_color");
   _u_flash_intensity_loc = get_loc("u_flash_intensity");
}
#else
void RingShaderLayer::checkUniforms(const std::string& shader_path)
{
   ShaderLayer::checkUniforms(shader_path);

   std::ifstream file(shader_path);
   if (!file.is_open())
   {
      return;
   }

   std::stringstream buffer;
   buffer << file.rdbuf();
   const auto shader_source = buffer.str();

   _has_u_ring_scale      = shader_source.find("u_ring_scale;")      != std::string::npos;
   _has_u_pixel_size      = shader_source.find("u_pixel_size;")      != std::string::npos;
   _has_u_flash_color     = shader_source.find("u_flash_color;")     != std::string::npos;
   _has_u_flash_intensity = shader_source.find("u_flash_intensity;") != std::string::npos;
}
#endif

void RingShaderLayer::readCustomProperties(const GameDeserializeData& data)
{
   const auto& map = data._tmx_object->_properties->_map;
   _ring_scale = ValueReader::readValue<float>("ring_scale", map).value_or(_ring_scale);
   _pixel_size = ValueReader::readValue<float>("pixel_size", map).value_or(_pixel_size);
}

#ifdef __EMSCRIPTEN__
void RingShaderLayer::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   draw(target, normal, {});
}

void RingShaderLayer::draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states)
{
   if (!_shader)
   {
      return;
   }

   if (_u_ring_scale_loc)
   {
      // NOTE: on WASM the ring renders larger than on desktop for the same ring_scale value;
      // root cause not yet found (see wasm_port_status.md). Left unmodified for now.
      _shader->setUniform(*_u_ring_scale_loc, _ring_scale);
   }
   if (_u_pixel_size_loc)
   {
      _shader->setUniform(*_u_pixel_size_loc, _pixel_size);
   }
   if (_u_flash_color_loc)
   {
      _shader->setUniform(*_u_flash_color_loc, _flash_color);
   }
   if (_u_flash_intensity_loc)
   {
      _shader->setUniform(*_u_flash_intensity_loc, _flash_intensity);
   }

   ShaderLayer::draw(target, normal, states);
}
#else
void RingShaderLayer::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   if (_has_u_ring_scale)
   {
      _shader.setUniform("u_ring_scale", _ring_scale);
   }

   if (_has_u_pixel_size)
   {
      _shader.setUniform("u_pixel_size", _pixel_size);
   }

   if (_has_u_flash_color)
   {
      _shader.setUniform("u_flash_color", _flash_color);
   }

   if (_has_u_flash_intensity)
   {
      _shader.setUniform("u_flash_intensity", _flash_intensity);
   }

   ShaderLayer::draw(target, normal);
}
#endif

void RingShaderLayer::update(const sf::Time& dt)
{
   ShaderLayer::update(dt);

   if (_flash_duration > 0.0f)
   {
      _flash_elapsed += dt.asSeconds();
      _flash_intensity = std::max(1.0f - _flash_elapsed / _flash_duration, 0.0f);
      if (_flash_elapsed >= _flash_duration)
      {
         _flash_duration = 0.0f;
         _flash_intensity = 0.0f;
      }
   }
}

void RingShaderLayer::setEnabled(bool enabled)
{
   if (!enabled)
   {
      _disable_time = std::chrono::high_resolution_clock::now();
   }

   ShaderLayer::setEnabled(enabled);
}

void RingShaderLayer::flash(float red, float green, float blue, float duration_s)
{
   _flash_color = sf::Glsl::Vec3{red, green, blue};
   _flash_duration = duration_s;
   _flash_elapsed = 0.0f;
}
