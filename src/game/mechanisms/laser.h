#pragma once

#include "framework/math/pathinterpolation.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

// sfml
#include "SFML/Graphics.hpp"

// box2d
#include "box2d/box2d.h"

// std
#include <array>
#include <filesystem>
#include <optional>
#include <vector>

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;

/// \brief controls laser hazard tiles with timed signals and optional path movement.
class Laser : public GameMechanism, public GameNode
{
public:
   /// \brief defines one timed on or off segment in a laser signal cycle.
   struct Signal
   {
      uint32_t _duration_ms = 0u;
      bool _on = false;
   };

   /// \brief stores movement settings for lasers that follow a path.
   struct Settings
   {
      float _movement_speed = 0.2f;
   };

   /// \brief creates a laser mechanism.
   /// \param parent parent node in the scene graph.
   Laser(GameNode* parent = nullptr);
   /// \brief returns the mechanism registry name.
   /// \return string view containing `Laser`.
   std::string_view objectName() const override;

   /// \brief draws the current laser frame from the laser tileset.
   /// \param color color render target.
   /// \param normal normal-map render target (unused).
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   /// \brief updates signal timing, frame animation, optional movement, and player collision checks.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;
   /// \brief returns the base laser tile rectangle.
   /// \return laser rectangle in pixels.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief creates one laser mechanism per non-empty tile in a TMX laser layer.
   /// \param parent parent node for created laser instances.
   /// \param data deserialize context with layer and tileset data.
   /// \return list of created laser mechanisms.
   static std::vector<std::shared_ptr<GameMechanism>> load(GameNode* parent, const GameDeserializeData& data);

   /// \brief stores a TMX object for later group merging and optional path assignment.
   /// \param object TMX object describing groups or movement paths.
   static void addObject(const std::shared_ptr<TmxObject>& object);
   /// \brief initializes the fine-collision bitmasks for version-1 laser tiles.
   static void addTilesVersion1();
   /// \brief initializes the fine-collision bitmasks for version-2 laser tiles.
   static void addTilesVersion2();
   /// \brief merges stored TMX objects into laser groups, signal plots, and movement paths.
   static void merge();

   /// \brief resets runtime state like animation frame and signal counters.
   void reset();
   /// \brief clears static staging data used while loading and merging laser groups.
   static void resetAll();

   /// \brief returns the tile position of this laser in tile units.
   /// \return tile position in tile units.
   const sf::Vector2f& getTilePosition() const;
   /// \brief returns the base world position of this laser in pixels.
   /// \return laser position in pixel coordinates.
   const sf::Vector2f& getPixelPosition() const;
   /// \brief returns the laser base collision rectangle in pixel space.
   /// \return laser rectangle in pixel coordinates.
   const sf::FloatRect& getPixelRect() const;

   /// \brief enables or disables signal-driven laser behavior.
   /// \param enabled true to allow normal on or off signal updates.
   void setEnabled(bool enabled) override;

protected:
   /// \brief performs rough and fine collision checks and kills the player on contact.
   void collide();

   std::vector<Signal> _signal_plot;

   int32_t _tu = 0;
   int32_t _tv = 0;

   std::shared_ptr<sf::Texture> _texture;
   std::unique_ptr<sf::Sprite> _sprite;

   sf::Vector2f _tile_position;
   sf::Vector2f _position_px;
   sf::FloatRect _pixel_rect;

   std::optional<std::vector<sf::Vector2f>> _path;
   sf::Vector2f _move_offset_px;
   PathInterpolation<sf::Vector2f> _path_interpolation;

   bool _on = true;
   int32_t _tile_index = 0;
   float _tile_animation = 0.0f;
   int32_t _animation_offset = 0;
   uint32_t _signal_index = 0;
   uint32_t _time = 0u;
   int32_t _group_id = 0;  // only for debugging purposes

   Settings _settings;
};
