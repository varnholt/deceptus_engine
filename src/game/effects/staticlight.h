#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

#include "gamedeserializedata.h"
#include "gamenode.h"

struct TmxObject;
struct TmxObjectGroup;

class StaticLight
{

public:

   static const std::string __layer_name;

   struct LightInstance : public GameNode
   {
      LightInstance(GameNode* parent)
       : GameNode(parent)
      {
         setClassName(typeid(LightInstance).name());
      }

      std::shared_ptr<sf::Texture> _texture;
      sf::Sprite _sprite;
      sf::BlendMode _blend_mode = sf::BlendAdd;
      sf::Color _color = {255, 255, 255, 255};
      int _z = 0;
      float _flicker_amount = 1.0f;
      float _flicker_intensity = 0.0f;
      float _flicker_speed = 0.0f;
      float _flicker_alpha_amount = 1.0f;
      float _time_offset = 0.0f;
   };

   std::vector<std::shared_ptr<LightInstance>> _lights;

   static std::shared_ptr<StaticLight::LightInstance> deserialize(GameNode* parent, const GameDeserializeData& data);


public:

   StaticLight() = default;

   void drawToZ(sf::RenderTarget& target, sf::RenderStates states, int z) const;
   void draw(sf::RenderTarget& target, sf::RenderStates states);
   void update(const sf::Time& time);
};
