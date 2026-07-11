#pragma once

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>
#include <optional>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

struct TmxObject;

/// \brief represents a physics-driven pushable box that the player can move.
class MoveableBox : public GameMechanism, public GameNode
{
public:
   /// \brief creates a moveable box mechanism.
   /// \param node parent node in the scene graph.
   MoveableBox(GameNode* node);

   /// \brief returns the mechanism registry name.
   /// \return string view containing `MoveableBox`.
   std::string_view objectName() const override;

   /// \brief preloads the looping push sound sample.
   void preload() override;

   /// \brief draws the moveable box sprite.
   /// \param color color render target.
   /// \param normal normal-map render target (unused).
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;

   /// \brief draws the moveable box sprite with explicit render states (used in WASM to carry the level view).
   /// \param color color render target.
   /// \param normal normal-map render target (unused).
   /// \param states render states to apply.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;

   /// \brief syncs sprite position from box2d and starts or stops pushing audio by velocity.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the current sprite bounds.
   /// \return box bounds in pixels.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief initializes sprite, physics settings, and dynamic box2d body from TMX object data.
   /// \param data deserialize context with TMX object and physics world.
   void setup(const GameDeserializeData& data);

   /// \brief stores configurable physics values read from TMX properties.
   struct Settings
   {
      float _density = 1.0f;
      float _friction = 0.0f;
      float _gravity_scale = 1.0f;
   };

private:
   /// \brief creates the dynamic beveled box body and collision fixture.
   /// \param world shared box2d world.
   void setupBody(const std::shared_ptr<b2World>& world);

   /// \brief initializes the body transform from the current sprite position.
   void setupTransform();

   std::shared_ptr<sf::Texture> _texture;
   std::unique_ptr<sf::Sprite> _sprite;
   sf::Vector2f _size;
   b2Body* _body = nullptr;
   std::optional<int32_t> _pushing_sample;
   Settings _settings;
};
