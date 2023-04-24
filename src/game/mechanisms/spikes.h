#pragma once

#include "gamedeserializedata.h"
#include "gamemechanism.h"
#include "gamenode.h"

#include "SFML/Graphics.hpp"

#include <Box2D/Box2D.h>
#include <filesystem>

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
      PointsUp,
      PointsDown,
      PointsRight,
      PointsLeft,
   };

   struct Config
   {
      int32_t _update_time_up_ms = 5;
      int32_t _update_time_down_ms = 30;
      int32_t _down_time_ms = 2000;
      int32_t _up_tims_ms = 2000;
      int32_t _trap_time_ms = 250;
   };

   Spikes(GameNode* parent = nullptr);

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   static std::vector<std::shared_ptr<Spikes>> load(GameNode* parent, const GameDeserializeData& data, Mode mode);
   static std::shared_ptr<Spikes> deserialize(GameNode* parent, const GameDeserializeData& data);

   const sf::FloatRect& getPixelRect() const;

   Mode getMode() const;
   void setMode(Mode mode);

private:
   void updateInterval();
   void updateTrap();
   void updateToggled();
   void updateSpriteRect();

   std::shared_ptr<sf::Texture> _texture;

   int32_t _tu = 0;
   int32_t _tv = 0;

   std::vector<sf::Sprite> _sprite;
   int32_t _elapsed_ms = 0;

   sf::Vector2f _pixel_position;
   sf::FloatRect _pixel_rect;

   bool _triggered = false;
   bool _deadly = false;

   Mode _mode = Mode::Invalid;
   Orientation _orientation = Orientation::Invalid;
   Config _config;
};
