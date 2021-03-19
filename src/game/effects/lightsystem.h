#pragma once

#include "effect.h"

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
      b2Vec2 _pos_m;
      float _intensity = 1.0f;
      sf::Color _color = {255, 255, 255, 80};
      bool _enabled = true;
      float max_dist_m2 = 40.0f;

      std::shared_ptr<sf::Texture> _texture;
      sf::Sprite _sprite;

      int _width_px = 256;
      int _height_px = 256;
      int _offset_x_px = 0;
      int _offset_y_px = 0;

      void updateSpritePosition();
   };

   std::vector<std::shared_ptr<LightInstance>> _lights;
   sf::Shader _light_shader;

   LightSystem();

   static std::shared_ptr<LightSystem::LightInstance> createLightInstance(TmxObject* tmxObject);

   void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
   void drawShadows(sf::RenderTarget &target, std::shared_ptr<LightInstance> light) const;
};

