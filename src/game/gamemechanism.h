#pragma once

#include "constants.h"

#include "SFML/Graphics.hpp"

#include <cstdint>


class GameMechanism
{
   public:

      GameMechanism() = default;
      virtual ~GameMechanism() = default;

      virtual void draw(sf::RenderTarget& target);
      virtual void update(const sf::Time& dt);

      virtual bool isEnabled() const;
      virtual void setEnabled(bool enabled);

      virtual int32_t getZ() const;
      virtual void setZ(const int32_t& z);

      virtual DrawMode getDrawMode() const;
      virtual void setDrawMode(DrawMode draw_mode);


   protected:

      int32_t mZ = 0;
      bool mEnabled = true;
      MechanismVersion mVersion = MechanismVersion::Version1;
      DrawMode _draw_mode = DrawMode::ColorMap;
};

