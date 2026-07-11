#pragma once

#include "game/animation/framemapper.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include "SFML/Graphics.hpp"

#include "box2d/box2d.h"
#include <filesystem>

struct TmxLayer;
struct TmxTileSet;

/// \brief animated spike strips that extend and retract to damage the player.
class Spikes : public GameMechanism, public GameNode
{
public:
   /// \brief behavior mode that controls when spikes extend.
   enum class Mode
   {
      Invalid,
      Trap,
      Interval,
      Toggled,
   };

   /// \brief orientation of spike graphics in the tileset.
   enum class Orientation
   {
      Invalid,
      PointsUp,
      PointsDown,
      PointsRight,
      PointsLeft,
   };

   /// \brief timing and speed configuration used by spike animation logic.
   struct Config
   {
      int32_t _down_time_ms{2000};
      int32_t _up_time_ms{2000};
      int32_t _trap_time_after_collision_ms{250};
      float _speed_up{35.0f};
      float _speed_down{35.0f};
   };

   /// \brief creates a spike mechanism instance with default state.
   /// \param parent owning game node in the scene graph.
   Spikes(GameNode* parent = nullptr);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "Spikes".
   std::string_view objectName() const override;

   /// \brief draws all spike sprites that belong to this mechanism.
   /// \param color color render target.
   /// \param normal normal-map render target, unused by this mechanism.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;

#ifdef __EMSCRIPTEN__
   /// \brief draws all spike sprites with explicit render states (used in WASM to carry the level view).
   /// \param color color render target.
   /// \param normal normal-map render target, unused by this mechanism.
   /// \param states render states to apply.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;
#endif

   /// \brief updates animation state and damages the player while spikes are deadly.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the player collision rectangle in pixel space.
   /// \return collision rectangle used for damage checks.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief toggles spike state in toggled mode and forwards enabled state to base class.
   /// \param enabled true to extend spikes in toggled mode.
   void setEnabled(bool enabled) override;

   /// \brief loads spikes from a tile layer and assigns a shared behavior mode.
   /// \param parent owning game node in the scene graph.
   /// \param data deserialization data for the tile layer and tileset.
   /// \param mode mode applied to all loaded spikes.
   /// \return list of created spike instances.
   static std::vector<std::shared_ptr<Spikes>> load(GameNode* parent, const GameDeserializeData& data, Mode mode);

   /// \brief creates one spike strip from an object and its custom properties.
   /// \param parent owning game node in the scene graph.
   /// \param data deserialization data with bounds, orientation, and timing values.
   /// \return configured spikes instance.
   static std::shared_ptr<Spikes> deserialize(GameNode* parent, const GameDeserializeData& data);

   /// \brief returns the rectangle used to test player collision and damage.
   /// \return collision rectangle in pixel space.
   const sf::FloatRect& getPixelRect() const;

   /// \brief returns the currently configured spike behavior mode.
   /// \return current mode.
   Mode getMode() const;

   /// \brief sets the spike behavior mode.
   /// \param mode new mode used during update.
   void setMode(Mode mode);

private:
   /// \brief updates extension and retraction for interval mode.
   void updateInterval();

   /// \brief updates trap trigger timing and one-shot extension behavior.
   void updateTrap();

   /// \brief updates extension and retraction for externally toggled mode.
   void updateToggled();

   /// \brief updates each sprite texture rectangle from the current animation frame.
   void updateSpriteRect();

   /// \brief updates whether the current frame should be treated as deadly.
   void updateDeadly();

   /// \brief computes a clamped tileset frame index from the animation cursor.
   /// \return current integer frame index.
   int32_t computeTuIndex();

   std::shared_ptr<sf::Texture> _texture;

   float _tu{0.0f};
   int32_t _tv{0};
   int32_t _tu_offset{0};

   std::vector<std::unique_ptr<sf::Sprite>> _sprite;
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
