#pragma once

#include "constants.h"

#include "SFML/Graphics.hpp"

#include <cstdint>


class GameMechanism
{
   public:

      GameMechanism() = default;
      virtual ~GameMechanism() = default;

      virtual void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
      virtual void update(const sf::Time& dt);

      virtual bool isEnabled() const;
      virtual void setEnabled(bool enabled);

      virtual int32_t getZ() const;
      virtual void setZ(const int32_t& z);


   protected:

      int32_t mZ = 0;
      bool mEnabled = true;
      MechanismVersion mVersion = MechanismVersion::Version1;
};

