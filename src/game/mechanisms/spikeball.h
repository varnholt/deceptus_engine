#pragma once

#include <SFML/Graphics.hpp>

#include "box2d/box2d.h"

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

struct TmxObject;

/// \brief simulates a chained swinging spike ball that kills the player on contact.
class SpikeBall : public GameMechanism, public GameNode
{
public:
   /// \brief physical and visual tuning values for chain and ball behavior.
   struct SpikeConfig
   {
      // factor to control the push force when ball moves from right to left
      float _ball_radius = 0.45f;
      float _push_factor = 0.625f;

      // number of points retrieved from the given spline
      int32_t _spline_point_count = 25;

      // chain element setup
      int32_t _chain_element_count = 10;
      float _chain_element_distance = 0.3f;
      float _chain_element_width = 0.06f;
      float _chain_element_height = 0.0125f;
   };

   /// \brief creates sprites, audio setup, and default chain fixture templates.
   /// \param parent owning game node in the scene graph.
   SpikeBall(GameNode* parent = nullptr);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "SpikeBall".
   std::string_view objectName() const override;

   /// \brief preloads swing direction-change sound effects.
   void preload() override;

   /// \brief draws the chain spline and spike ball sprite.
   /// \param color color render target.
   /// \param normal normal-map render target, unused by this mechanism.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;

#ifdef __EMSCRIPTEN__
   /// \brief draws the chain spline and spike ball sprite with explicit render states (used in WASM to carry the level view).
   /// \param color color render target.
   /// \param normal normal-map render target, unused by this mechanism.
   /// \param states render states to apply.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;
#endif

   /// \brief updates ball pose, swing audio, and optional push impulse.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the map-defined mechanism bounds in pixel space.
   /// \return rectangle used for chunk activation.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief creates chain bodies, joints, and deadly ball fixture from tmx settings.
   /// \param data deserialization data with world pointer and spikeball properties.
   void setup(const GameDeserializeData& data);

   /// \brief returns the anchor position in pixels.
   /// \return pixel position used as the spikeball origin.
   sf::Vector2i getPixelPosition() const;

   /// \brief sets the anchor position in pixels.
   /// \param pixel_position position in pixels.
   void setPixelPosition(const sf::Vector2i& pixel_position);

private:
#ifdef __EMSCRIPTEN__
   /// \brief draws interpolated chain segments between box2d chain bodies.
   /// \param window render target window.
   /// \param states render states to apply.
   void drawChain(sf::RenderTarget& window, const sf::RenderStates& states);
#else
   /// \brief draws interpolated chain segments between box2d chain bodies.
   /// \param window render target window.
   void drawChain(sf::RenderTarget& window);
#endif

   std::shared_ptr<sf::Texture> _texture;
   std::unique_ptr<sf::Sprite> _spike_sprite;
   std::unique_ptr<sf::Sprite> _box_sprite;
   std::unique_ptr<sf::Sprite> _chain_element_a;
   std::unique_ptr<sf::Sprite> _chain_element_b;

   sf::Vector2i _pixel_position;
   sf::FloatRect _rect;

   b2BodyDef _anchor_def;
   b2Body* _anchor_body = nullptr;
   b2EdgeShape _anchor_shape;

   b2RevoluteJointDef _joint_def;
   b2PolygonShape _chain_element_shape;
   b2FixtureDef _chain_element_fixture_def;
   std::vector<b2Body*> _chain_elements;

   b2Body* _ball_body = nullptr;
   b2CircleShape _ball_shape;
   b2BodyDef _ball_body_def;
   b2FixtureDef _ball_fixture_def;

   float _angle = 0.0f;
   float _last_ball_x_velocity = 0.0f;
   int32_t _swing_counter = 0;
   SpikeConfig _config;
   int32_t _instance_id{0};
};
