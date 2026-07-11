#pragma once

#include "game/animation/animation.h"
#include "game/constants.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <filesystem>

#include "SFML/Graphics.hpp"

#include "box2d/box2d.h"

struct TmxLayer;
struct TmxTileSet;

/// \brief controls an interactable door that can block or allow passage.
class Door : public GameMechanism, public GameNode
{
public:
   enum class Version
   {
      Version1,
      Version2,
   };

   enum class State
   {
      Open,
      Opening,
      Closing,
      Closed,
   };

   /// \brief creates a door mechanism.
   /// \param parent parent node in the scene graph.
   Door(GameNode* parent);

   /// \brief destroys the door mechanism.
   ///
   /// no special runtime cleanup is performed here.
   virtual ~Door();

   /// \brief returns the mechanism registry name.
   /// \return string view containing `Door`.
   std::string_view objectName() const override;

   /// \brief draws the current door visual state and optional key hint animation.
   /// \param color color render target.
   /// \param normal normal-map render target (unused).
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;

   /// \brief draws the current door visual state with explicit render states (used in WASM to carry the level view).
   /// \param color color render target.
   /// \param normal normal-map render target (unused).
   /// \param states render states to apply.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;

   /// \brief updates animations, body enable state, and player interaction with button b.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief maps mechanism enabled state to door motion.
   /// \param enabled true to open the door, false to close it.
   void setEnabled(bool enabled) override;

   /// \brief returns the door bounds used for mechanism queries.
   /// \return door rectangle in pixels.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief starts the opening sequence, including observer, audio, and animation events.
   void open();

   /// \brief starts the closing sequence when this door is allowed to close.
   void close();

   /// \brief toggles between open and closed states when no transition is running.
   void toggle() override;

   /// \brief initializes properties, visuals, interaction range, and physics body from TMX data.
   /// \param data deserialize context with TMX object data and box2d world.
   /// \return true when TMX object data is available and setup completed.
   bool setup(const GameDeserializeData& data);

   /// \brief reports whether the door currently treats the player as nearby.
   /// \return true when the player-at-door flag is set.
   bool isPlayerAtDoor() const;

   /// \brief updates the cached nearby-player flag used for ui and audio behavior.
   /// \param isPlayerAtDoor true when the player is considered near the door.
   void setPlayerAtDoor(bool isPlayerAtDoor);

   /// \brief returns the top-left tile coordinate of the door.
   /// \return tile position in tile units.
   const sf::Vector2i& getTilePosition() const;

   /// \brief returns the door rectangle in pixel coordinates.
   /// \return door rectangle in pixel coordinates.
   const sf::FloatRect& getPixelRect() const;

private:
   /// \brief creates the kinematic box2d body used to block player movement.
   /// \param world shared box2d world.
   void setupBody(const std::shared_ptr<b2World>& world);

   /// \brief updates body transform so collision follows the animated door offset.
   void updateTransform();

   /// \brief updates legacy version-1 bar movement and its collision transform.
   /// \param dt elapsed frame time.
   void updateBars(const sf::Time& dt);

   /// \brief checks whether the player's pixel position lies inside the interaction area.
   /// \return true when the player is inside the interaction rectangle.
   bool checkPlayerAtDoor() const;

   std::unique_ptr<sf::Sprite> _sprite;
   std::shared_ptr<sf::Texture> _texture;
   std::optional<std::string> _sample_open;
   std::optional<std::string> _sample_close;
   std::shared_ptr<Animation> _animation_open;
   std::shared_ptr<Animation> _animation_close;
   std::shared_ptr<Animation> _animation_key;
   sf::FloatRect _player_at_door_rect;

   Version _version = Version::Version2;
   State _state = State::Closed;

   // for 'version 1'
   sf::VertexArray _door_quad{sf::PrimitiveType::Triangles, 4};
   sf::Vector2i _tile_position_tl;
   sf::FloatRect _pixel_rect;
   float _bar_offset = 0.0f;

   std::optional<std::string> _required_item;

   bool _can_be_closed = false;
   bool _automatic_close = false;
   std::optional<std::chrono::high_resolution_clock::time_point> _last_toggle_time;

   bool _player_at_door = false;
   b2Body* _body = nullptr;
};
