#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

struct TmxObject;
struct TmxObjectGroup;

/// \brief draws a textured additive light with optional procedural flicker.
class StaticLight : public GameMechanism, public GameNode
{
public:
   /// \brief creates a static light mechanism instance.
   /// \param parent owning game node in the scene graph.
   StaticLight(GameNode* parent = nullptr);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "StaticLight".
   std::string_view objectName() const override;

   /// \brief draws the light sprite using additive blending and flicker-adjusted alpha.
   /// \param target render target.
   /// \param color secondary render target, unused by this mechanism.
   void draw(sf::RenderTarget& target, sf::RenderTarget& color) override;

   /// \brief draws the light sprite with explicit render states (used in WASM to carry the level view).
   /// \param target render target.
   /// \param color secondary render target, unused by this mechanism.
   /// \param states render states to apply.
   void draw(sf::RenderTarget& target, sf::RenderTarget& color, const sf::RenderStates& states) override;
   using GameMechanism::draw;

   /// \brief updates procedural flicker amount from global time.
   /// \param time elapsed frame time, unused directly.
   void update(const sf::Time& time) override;

   /// \brief reports that this mechanism has no gameplay collision bounds.
   /// \return std::nullopt.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief loads texture, color, flicker, and bounds from tmx properties.
   /// \param data deserialization data with light configuration values.
   void deserialize(const GameDeserializeData& data);

   std::shared_ptr<sf::Texture> _texture;
   std::unique_ptr<sf::Sprite> _sprite;
   sf::BlendMode _blend_mode = sf::BlendAdd;
   sf::Color _color = {255, 255, 255, 255};
   float _flicker_amount = 1.0f;
   float _flicker_intensity = 0.0f;
   float _flicker_speed = 0.0f;
   float _flicker_alpha_amount = 1.0f;
   float _time_offset = 0.0f;
   int32_t _instance_number = 0;
   sf::FloatRect _rect;
};
