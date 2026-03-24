#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include "box2d/box2d.h"

#include <cstdint>

class GameNode;
struct TmxObject;

/// \brief simulates and renders a swinging rope built from box2d chain segments.
class Rope : public GameMechanism, public GameNode
{
public:
   /// \brief creates a rope mechanism with default segment and wind settings.
   /// \param parent owning game node in the scene graph.
   Rope(GameNode* parent);
   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "Rope".
   std::string_view objectName() const override;

   /// \brief draws the rope as a textured strip between simulated chain bodies.
   /// \param color color render target.
   /// \param normal normal-map render target, unused by this mechanism.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   /// \brief updates wind impulses, player influence, and rope motion.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;
   /// \brief returns the interaction area used for player impulse transfer.
   /// \return rope bounding rectangle in pixel space.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief builds rope geometry, reads tmx properties, and creates box2d joints.
   /// \param data deserialization data with polyline path, world, and properties.
   virtual void setup(const GameDeserializeData& data);

   /// \brief returns the rope anchor position in pixels.
   /// \return anchor position used to initialize the rope chain.
   sf::Vector2i getPixelPosition() const;
   /// \brief sets the rope anchor position in pixels.
   /// \param pixel_position anchor position used for box2d body creation.
   void setPixelPosition(const sf::Vector2i& pixel_position);

protected:
   int32_t _segment_count = 7;
   float _segment_length_m = 0.01f;

   std::vector<b2Body*> _chain_elements;
   std::shared_ptr<sf::Texture> _texture;

private:
   /// \brief applies a horizontal impulse to all chain elements.
   /// \param impulse impulse strength applied in world units.
   void pushChain(float impulse);

   sf::Vector2i _position_px;
   sf::FloatRect _bounding_box;

   // attachment of the 1st end of the rope
   b2BodyDef _anchor_a_def;
   b2Body* _anchor_a_body = nullptr;
   b2EdgeShape _anchor_a_shape;
   std::vector<b2Body*> _chain_bodies;

   // rope
   b2PolygonShape _rope_element_shape;
   b2FixtureDef _rope_element_fixture_def;
   b2RevoluteJointDef _joint_def;

   sf::IntRect _texture_rect_px;

   // wind
   bool _wind_enabled = true;
   float _push_time_s = 0.0f;
   float _push_interval_s = 5.0f;
   float _push_duration_s = 1.0f;
   float _push_strength = 0.02f;
   std::optional<float> _player_impulse;

   static int32_t _instance_counter;
};
