#pragma once

#include <array>

#include "game/effects/lightsystem.h"
#include "game/io/gamedeserializedata.h"
#include "game/mechanisms/rope.h"

/// \brief extends rope with a hanging lamp sprite and dynamic light source.
class RopeWithLight : public Rope
{
public:
   /// \brief creates a rope-with-light mechanism instance.
   /// \param parent owning game node in the scene graph.
   RopeWithLight(GameNode* parent);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "RopeWithLight".
   std::string_view objectName() const override;

   /// \brief draws the rope and then draws the attached lamp sprite.
   /// \param color color render target.
   /// \param normal normal-map render target forwarded to rope drawing.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;

   /// \brief updates rope physics and aligns lamp sprite and light to the rope end.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief initializes rope physics, lamp visuals, and light properties from tmx data.
   /// \param data deserialization data with rope and lamp configuration.
   void setup(const GameDeserializeData& data) override;

private:
   std::unique_ptr<sf::Sprite> _lamp_sprite;
   std::array<sf::IntRect, 3> _lamp_sprite_rects;
   std::shared_ptr<LightSystem::LightInstance> _light;
};
