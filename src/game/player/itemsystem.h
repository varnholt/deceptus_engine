#pragma once

#include <memory>
#include <vector>
#include <array>
#include <string>

#include "game/items/item.h"

/// \brief manages instantiated usable items and keeps equipped item slots in sync with inventory slots.
class ItemSystem
{
public:
   /// \brief creates an empty runtime item system.
   ItemSystem() = default;

   /// \brief updates all currently equipped item instances.
   /// \param dt elapsed frame time forwarded to each equipped item.
   void update(const sf::Time& dt);
   /// \brief draws all currently equipped item instances.
   /// \param target render target passed to each equipped item.
   void draw(sf::RenderTarget& target);
   /// \brief ensures a runtime item instance exists for an inventory item key.
   /// \param item_name inventory item key to instantiate.
   /// \return existing or newly created item instance, or nullptr when the item key is unsupported.
   std::shared_ptr<Item> onInventoryItemAdded(const std::string& item_name);
   /// \brief removes a runtime item instance and unequips it from any slot.
   /// \param item_name inventory item key to remove.
   void onInventoryItemRemoved(const std::string& item_name);
   /// \brief synchronizes equipped runtime slots to match inventory slot names.
   /// \param slots two inventory slot item keys, where empty entries clear item slots.
   void syncInventorySlots(const std::array<std::string, 2>& slots);

private:

   std::array<std::shared_ptr<Item>, 2> _slots;
   std::vector<std::shared_ptr<Item>> _items;
};
