#pragma once

#include "game/gamemechanism.h"
#include "game/level/gamenode.h"
#include "gamedeserializedata.h"

#include <SFML/Graphics.hpp>
#include <array>
#include <deque>
#include <memory>

struct TmxObject;

class Dust : public GameMechanism, public GameNode
{
   struct Particle
   {
      void spawn(sf::FloatRect& rect);
      sf::Vector3f _position;
      sf::Vector3f _direction;
      float _age = 0.0f;
      float _lifetime = 0.0f;
      float _z = 0.0f;
   };

public:
   Dust(GameNode* parent = nullptr);

   void update(const sf::Time& dt) override;
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   static std::shared_ptr<Dust> deserialize(GameNode* parent, const GameDeserializeData& data);

private:
   std::vector<Particle> _particles;
   sf::FloatRect _clip_rect;
   std::shared_ptr<sf::Texture> _flow_field_texture;
   sf::Image _flow_field_image;
   sf::Vector3f _wind_direction;
   sf::Color _particle_color = {255, 255, 255, 255};
   float _particle_velocity = 100.0f;
   uint8_t _particle_size_px = 2;
};
