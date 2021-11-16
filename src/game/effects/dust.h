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
      sf::Texture _flow_field;
};

