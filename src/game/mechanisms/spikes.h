#pragma once

#include "framemapper.h"
#include "gamedeserializedata.h"
#include "gamemechanism.h"
#include "gamenode.h"

#include "SFML/Graphics.hpp"

#include <box2d/box2d.h>
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
      int32_t _down_time_ms{2000};
      int32_t _up_time_ms{2000};
      int32_t _trap_time_after_collision_ms{250};
      float _speed_up{35.0f};
      float _speed_down{35.0f};
   };

   Spikes(GameNode* parent = nullptr);

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;
   void setEnabled(bool enabled) override;

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
   void updateDeadly();

   std::shared_ptr<sf::Texture> _texture;

   float _tu{0.0f};
   int32_t _tv{0};
   int32_t _tu_offset{0};

   std::vector<sf::Sprite> _sprite;
   int32_t _elapsed_ms{0};
   float _dt_s{0.0f};
   int32_t _dt_ms{0};
   std::optional<int32_t> _elapsed_since_collision_ms;

   sf::Vector2f _pixel_position;
   sf::FloatRect _player_collision_rect_px;

   bool _extracting{false};
   bool _deadly{false};
   std::optional<int32_t> _idle_time_ms;

   Mode _mode = Mode::Invalid;
   Orientation _orientation = Orientation::Invalid;
   Config _config;
   int32_t _instance_id{0};
};
