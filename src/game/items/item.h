#pragma once

#include <SFML/Graphics.hpp>
#include <string>

/// \brief abstract base type for equippable player items.
class Item
{
public:
   /// \brief creates an item with default state.
   Item() = default;
   /// \brief destroys the item instance.
   virtual ~Item() = default;

   /// \brief draws item-specific visuals.
   /// \param target SFML render target that receives the item graphics.
   virtual void draw(sf::RenderTarget& target);
   /// \brief updates item state for the current frame.
   /// \param dt elapsed frame time since the previous update.
   virtual void update(const sf::Time& dt);
   /// \brief handles the item being equipped by the player.
   virtual void onEquipped();
   /// \brief handles the item being unequipped by the player.
   virtual void onUnequipped();
   /// \brief gets the display name of this item.
   /// \return localized or user-facing item name.
   virtual std::string getName() const = 0;
};
