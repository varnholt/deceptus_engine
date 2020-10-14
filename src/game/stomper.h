#pragma once

#include "constants.h"
#include "gamemechanism.h"

#include <filesystem>

#include <Box2D/Box2D.h>


struct TmxLayer;
struct TmxTileSet;

class Stomper : public GameMechanism
{
   public:

      enum class Mode
      {
         Interval,
         Distance
      };

      Stomper();


      static std::vector<std::shared_ptr<GameMechanism> > load(
         TmxLayer* layer,
         TmxTileSet* tileSet,
         const std::filesystem::path& basePath,
         const std::shared_ptr<b2World>&
      );


   private:

      Mode mMode = Mode::Distance;
      Alignment mAlignment = Alignment::PointsDown;

      sf::Vector2i mPosition;
};

