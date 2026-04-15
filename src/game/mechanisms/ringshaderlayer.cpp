#include "ringshaderlayer.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "game/io/valuereader.h"

#include <algorithm>
#include <fstream>
#include <sstream>

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

void RingShaderLayer::readCustomProperties(const GameDeserializeData& data)
{
   const auto& map = data._tmx_object->_properties->_map;
   _ring_scale = ValueReader::readValue<float>("ring_scale", map).value_or(_ring_scale);
   _pixel_size = ValueReader::readValue<float>("pixel_size", map).value_or(_pixel_size);
}

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

void RingShaderLayer::update(const sf::Time& dt)
{
   ShaderLayer::update(dt);

   if (_flash_duration > 0.0f)
   {
      _flash_elapsed += dt.asSeconds();
      _flash_intensity = std::max(1.0f - _flash_elapsed / _flash_duration, 0.0f);
      if (_flash_elapsed >= _flash_duration)
      {
         _flash_duration  = 0.0f;
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
   _flash_color    = sf::Glsl::Vec3{red, green, blue};
   _flash_duration = duration_s;
   _flash_elapsed  = 0.0f;
}
