#include "ringshaderlayer.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "game/io/valuereader.h"

#include <algorithm>

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

void RingShaderLayer::readCustomProperties(const GameDeserializeData& data)
{
   const auto& map = data._tmx_object->_properties->_map;
   _ring_scale = ValueReader::readValue<float>("ring_scale", map).value_or(_ring_scale);
   _pixel_size = ValueReader::readValue<float>("pixel_size", map).value_or(_pixel_size);
}

void RingShaderLayer::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   if (!_shader)
   {
      return;
   }

   if (_u_ring_scale_loc)
   {
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
