#pragma once

#include "game/items/item.h"

#include <SFML/Graphics.hpp>

/// \brief lantern item that renders a warm circular player light when equipped.
class ItemLantern : public Item
{
public:
   /// \brief initializes the lantern light shape and default radius.
   ItemLantern();

   /// \brief draws the lantern light circle when the item is enabled.
   /// \param target SFML render target that receives the light sprite.
   void draw(sf::RenderTarget& target) override;
   /// \brief updates the light position to follow the current player.
   /// \param dt elapsed frame time since the previous update.
   void update(const sf::Time& dt) override;
   /// \brief enables lantern rendering and updates while equipped.
   void onEquipped() override;
   /// \brief disables lantern rendering and updates while unequipped.
   void onUnequipped() override;
   /// \brief gets the inventory display name for this item.
   /// \return the string "Lantern".
   std::string getName() const override;

private:
   sf::CircleShape _light_circle;
   float _light_radius;
   bool _enabled{false};
};
