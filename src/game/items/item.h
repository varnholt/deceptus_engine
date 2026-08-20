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
   /// \param states render states applied to the item graphics (carries .view for WASM camera transform).
   virtual void draw(sf::RenderTarget& target, const sf::RenderStates& states);

   /// \brief updates item state for the current frame.
   /// \param dt elapsed frame time since the previous update.
   virtual void update(const sf::Time& dt);

   /// \brief places this item's sprites for the frame about to be drawn.
   /// \note items ride on the player, which is placed once per frame from an interpolated position,
   ///       so an item placed once per simulation step would sit a step away from it.
   virtual void updateSpritePositions();

   /// \brief handles the item being equipped by the player.
   virtual void onEquipped();

   /// \brief handles the item being unequipped by the player.
   virtual void onUnequipped();

   /// \brief gets the display name of this item.
   /// \return localized or user-facing item name.
   virtual std::string getName() const = 0;
};
