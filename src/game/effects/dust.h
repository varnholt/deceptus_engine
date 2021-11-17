#pragma once


#include "game/gamemechanism.h"

#include <SFML/Graphics.hpp>
#include <array>
#include <deque>


struct TmxObject;

class Dust : public GameMechanism
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

      Dust() = default;

      void update(const sf::Time& dt) override;
      void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

      static std::shared_ptr<Dust> deserialize(TmxObject* tmx_object);


   private:

      std::vector<Particle> _particles;
      sf::FloatRect _clip_rect;
      sf::Image _flow_field_image;
      sf::Vector3f _wind_direction;
      sf::Color _particle_color = {255, 255, 255, 255};
      float _particle_velocity = 100.0f;
      uint8_t _particle_size_px = 2;
};

