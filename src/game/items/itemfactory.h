#pragma once

#include <memory>
#include <string>

class Item;

/// \brief creates item instances from inventory item names.
class ItemFactory
{
public:
   /// \brief looks up an item by name and returns a shared item instance.
   /// \param item_name item identifier from save data or UI input.
   /// \return shared item instance when the name is supported, otherwise nullptr.
   static std::shared_ptr<Item> create(const std::string& item_name);
};
