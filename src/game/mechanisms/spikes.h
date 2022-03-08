#pragma once

#include "gamedeserializedata.h"
#include "gamemechanism.h"
#include "gamenode.h"

#include "SFML/Graphics.hpp"

#include <filesystem>
#include <Box2D/Box2D.h>

struct TmxLayer;
struct TmxTileSet;

class Spikes : public GameMechanism, public GameNode
{
public:

   enum class Mode
   {
      Invalid,
      Trap,
      Interval,
      Toggled,
   };

   enum class Orientation
   {
      Invalid,
      PointsTop,
      PointsLeft,
      PointsUp,
      PointsDown
   };

   Spikes(GameNode* parent = nullptr);

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;

   static std::vector<std::shared_ptr<Spikes>> load(
      GameNode* parent,
      const GameDeserializeData& data,
      Mode mode
   );

   const sf::IntRect& getPixelRect() const;

   Mode getMode() const;
   void setMode(Mode mode);


private:

   void updateInterval();
   void updateTrap();
   void updateToggled();
   void updateSpriteRect();

   sf::Vector2u _tile_size;

   std::shared_ptr<sf::Texture> _texture;

   int32_t _tu = 0;
   int32_t _tv = 0;

   sf::Sprite _sprite;
   int32_t _elapsed_ms = 0;

   sf::Vector2f _tile_position;
   sf::Vector2f _pixel_position;
   sf::IntRect _pixel_rect;

   bool _triggered = false;
   bool _deadly = false;

   Mode _mode = Mode::Invalid;
   Orientation _orientation = Orientation::Invalid;
};

