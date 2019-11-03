#pragma once

#include "SFML/Graphics.hpp"

#include <cstdint>


class GameMechanism
{
   public:

      GameMechanism() = default;
      virtual ~GameMechanism() = default;

      int32_t getZ() const;
      void setZ(const int32_t& z);

      virtual void draw(sf::RenderTarget& target) = 0;
      virtual void update(const sf::Time& dt) = 0;


   protected:

      int32_t mZ = 0;
};

