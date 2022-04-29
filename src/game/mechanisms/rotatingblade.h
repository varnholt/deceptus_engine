#pragma once

#include "gamemechanism.h"
#include "gamenode.h"

#include "framework/math/pathinterpolation.h"
#include "gamedeserializedata.h"

#include "SFML/Graphics.hpp"
#include <vector>


class RotatingBlade : public GameMechanism, public GameNode
{
public:

   struct Settings
   {
      float _blade_acceleration = 0.003f;
      float _blade_deceleration = 0.003f;
      float _blade_rotation_speed = 0.1f;
   };

   enum class PathType
   {
      Polyline,
      Polygon
   };

   RotatingBlade(GameNode* parent = nullptr);

   void setup(const GameDeserializeData& data);
   void update(const sf::Time& dt) override;
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void setEnabled(bool enabled) override;

private:
   float _angle = 0.0f;
   float _velocity = 0.0f;
   float _direction = 1.0f;
   float _time_normalized = 0.0f;

   std::shared_ptr<sf::Texture> _texture_map;
   std::shared_ptr<sf::Texture> _normal_map;
   sf::Sprite _sprite;
   sf::IntRect _rectangle;
   std::vector<sf::Vector2f> _path;
   PathInterpolation<sf::Vector2f> _path_interpolation;
   PathType _path_type = PathType::Polygon;
   Settings _settings;
};
