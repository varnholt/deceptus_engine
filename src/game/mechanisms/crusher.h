#pragma once

#include "game/constants.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include "box2d/box2d.h"

struct TmxObject;

/// \brief animated spike trap that periodically extends and retracts to crush the player.
class Crusher : public GameMechanism, public GameNode
{
public:
   enum class Mode
   {
      Interval,
      Distance
   };

   enum class State
   {
      Idle,
      Extract,
      Retract
   };

   /// \brief initializes crusher sprites and assigns a unique instance id.
   /// \param parent owning game node in the scene graph.
   Crusher(GameNode* parent = nullptr);

   /// \brief returns the mechanism type identifier.
   /// \return non-owning string view with value "Crusher".
   std::string_view objectName() const override;

   /// \brief draws crusher mount, pusher, and spike sprites.
   /// \param color color render target.
   /// \param normal normal render target.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;

   /// \brief draws crusher mount, pusher, and spike sprites with explicit render states (used in WASM to carry the level view).
   /// \param color color render target.
   /// \param normal normal render target.
   /// \param states render states to apply.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;

   /// \brief updates crusher state machine, motion, and collision transform.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the configured crusher area in pixel coordinates.
   /// \return rectangle used for culling and editor bounds.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief initializes geometry, timing, alignment, sprites, and body from tmx properties.
   /// \param data deserialize context with tmx object and physics world.
   void setup(const GameDeserializeData& data);

private:
   /// \brief updates body transform and velocity to follow the animated blade position.
   void updateTransform();

   /// \brief builds kinematic crusher fixtures for deadly spike and push volume.
   /// \param world physics world that owns the crusher body.
   void setupBody(const std::shared_ptr<b2World>& world);

   /// \brief advances blade offset for the current state using easing functions.
   /// \param dt elapsed frame time.
   void step(const sf::Time& dt);

   /// \brief advances the interval state machine between idle, extract, and retract.
   void updateState();

   /// \brief updates sprite scaling and positions to match current blade offset.
   void updateSpritePositions();

   /// \brief triggers a camera shake boom once during extraction when allowed.
   void startBoomEffect();

   /// \brief resets the boom trigger guard after retraction has progressed.
   void stopBoomEffect();

   Mode _mode = Mode::Interval;
   State _state = State::Idle;
   State _state_previous = State::Idle;
   Alignment _alignment = Alignment::PointsDown;

   b2Body* _body{nullptr};
   sf::Vector2f _pixel_position;
   sf::Vector2f _blade_offset;
   sf::FloatRect _rect;

   sf::Time _idle_time;
   sf::Time _extraction_time;
   sf::Time _retraction_time;
   sf::Time _idle_time_max;
   sf::Time _extraction_time_max;
   sf::Time _retraction_time_max;
   sf::Time _time_offset;

   std::unique_ptr<sf::Sprite> _sprite_spike;
   std::unique_ptr<sf::Sprite> _sprite_pusher;
   std::unique_ptr<sf::Sprite> _sprite_mount;
   sf::Vector2f _pixel_offset_mount;
   sf::Vector2f _pixel_offset_pusher;
   sf::Vector2f _pixel_offset_spike;

   bool _shake{true};
   bool _shake_shown{false};

   int32_t _instance_id{0};

   std::shared_ptr<sf::Texture> _texture;
   static int32_t __instance_counter;
};
