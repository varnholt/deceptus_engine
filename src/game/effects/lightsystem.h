#pragma once

#include <array>
#include <memory>
#include <vector>

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>


struct TmxObject;

class LightSystem : public sf::Drawable
{

public:

   struct LightInstance
   {
      b2Vec2 _pos_m = b2Vec2{0.0f, 0.0f};
      b2Vec2 _center_offset_m = b2Vec2{0.0f, 0.0f};
      sf::Vector2i _center_offset_px;

      sf::Color _color = {255, 255, 255, 80};
      std::array<float, 3> _falloff = {0.4f, 3.0f, 20.0f};

      bool _enabled = true;
      float max_dist_m2 = 40.0f;

      std::shared_ptr<sf::Texture> _texture;
      sf::Sprite _sprite;

      int32_t _width_px = 256;
      int32_t _height_px = 256;

      void updateSpritePosition();
   };

   std::vector<std::shared_ptr<LightInstance>> _lights;
   sf::Shader _light_shader;
   void increaseAmbient(float amount);
   void decreaseAmbient(float amount);

   LightSystem();

   static std::shared_ptr<LightSystem::LightInstance> createLightInstance(TmxObject* tmxObject = nullptr);

   void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

   void draw(
      sf::RenderTarget& target,
      const std::shared_ptr<sf::RenderTexture>& color_map,
      const std::shared_ptr<sf::RenderTexture>& light_map,
      const std::shared_ptr<sf::RenderTexture>& normal_map
   );


private:

   void drawShadowQuads(sf::RenderTarget &target, std::shared_ptr<LightInstance> light) const;
   void updateLightShader(sf::RenderTarget& target);

   mutable std::vector<std::shared_ptr<LightInstance>> _active_lights;

   std::array<float, 4> _ambient_color = {1.0f, 1.0f, 1.0f, 1.0f};
   static constexpr auto segments = 20;
   std::array<b2Vec2, segments> _unit_circle;
};

