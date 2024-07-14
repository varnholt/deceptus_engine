#pragma once

#include "framework/math/pathinterpolation.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <vector>
#include "SFML/Graphics.hpp"

class RotatingBlade : public GameMechanism, public GameNode
{
public:
   struct Settings
   {
      float _blade_acceleration = 0.006f;
      float _blade_deceleration = 0.009f;
      float _blade_rotation_speed = 400.0f;
      float _movement_speed = 0.2f;
   };

   enum class PathType
   {
      Polyline,
      Polygon
   };

   RotatingBlade(GameNode* parent = nullptr);
   ~RotatingBlade();

   void setup(const GameDeserializeData& data);
   void preload() override;
   void update(const sf::Time& dt) override;
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void setAudioEnabled(bool enabled) override;
   void setReferenceVolume(float volume) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   const sf::FloatRect& getPixelRect() const;

private:
   void updateAudio();

   float _angle = 0.0f;
   float _velocity = 0.0f;
   float _direction = 1.0f;

   std::shared_ptr<sf::Texture> _texture_map;
   std::shared_ptr<sf::Texture> _normal_map;
   sf::Sprite _sprite;
   sf::FloatRect _rectangle;
   std::vector<sf::Vector2f> _path;
   sf::Vector2f _pos;
   PathInterpolation<sf::Vector2f> _path_interpolation;
   PathType _path_type = PathType::Polygon;
   Settings _settings;
   std::optional<int32_t> _sample_enabled;
   std::optional<int32_t> _sample_accelerate;
   std::optional<int32_t> _sample_decelerate;
};
