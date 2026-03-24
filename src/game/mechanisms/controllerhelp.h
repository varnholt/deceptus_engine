#pragma once

#include <array>
#include <memory>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

struct TmxObject;

/// \brief on-screen input hint that fades in near the player and swaps keyboard/controller icons.
class ControllerHelp : public GameMechanism, public GameNode
{
public:
   /// \brief creates an empty controller-help mechanism.
   /// \param parent parent node in the scene graph.
   ControllerHelp(GameNode* parent = nullptr);
   /// \brief returns the mechanism type identifier.
   /// \return non-owning string view with value "ControllerHelp".
   std::string_view objectName() const override;
   /// \brief draws background and mapped key icons with current fade and bob animation.
   /// \param target render target.
   /// \param normal normal render target.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   /// \brief updates visibility and alpha based on player overlap with the help area.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;
   /// \brief initializes trigger area and icon mappings from tmx properties.
   /// \param data deserialize context containing object rect and key list.
   void deserialize(const GameDeserializeData& data);
   /// \brief returns the area that controls hint visibility.
   /// \return trigger rectangle in pixel coordinates.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

private:
   std::shared_ptr<sf::Texture> _texture;
   sf::FloatRect _rect_px;
   sf::Vector2f _rect_center;

   std::vector<sf::Sprite> _sprites;
   std::vector<sf::IntRect> _sprite_rects_keyboard;
   std::vector<sf::IntRect> _sprite_rects_controller;

   std::unique_ptr<sf::Sprite> _background;
   float _alpha = 0.0f;
   sf::Time _time;
};
