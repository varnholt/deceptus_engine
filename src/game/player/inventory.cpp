#include "inventory.h"

#include <ranges>

using json = nlohmann::json;

Inventory::Inventory()
{
   _descriptions = InventoryItemDescriptionReader::readItemDescriptions();
}

void Inventory::add(const std::string& item)
{
   _items.push_back(item);

   // fill empty slots with new items
   autoPopulate(item);

   std::ranges::for_each(_updated_callbacks, [](const auto& cb) { cb(); });
   std::ranges::for_each(_added_callbacks, [&item](const auto& cb) { cb(item); });
}

void Inventory::remove(const std::string& item)
{
   auto it = std::remove(_items.begin(), _items.end(), item);
   const auto removed_count = static_cast<size_t>(std::distance(it, _items.end()));
   _items.erase(it, _items.end());

   auto slot_cleared = false;

   // remove item from slots
   std::ranges::for_each(
      _slots,
      [item, &slot_cleared](auto& slot)
      {
         if (slot == item)
         {
            slot.clear();
            slot_cleared = true;
         }
      }
   );

   if (removed_count > 0 || slot_cleared)
   {
      std::ranges::for_each(_removed_callbacks, [&item](const auto& cb) { cb(item); });
   }

   // notify callbacks
   std::ranges::for_each(_updated_callbacks, [](const auto& cb) { cb(); });
}

void Inventory::clear()
{
   const auto removed_items = _items;
   _items.clear();
   std::ranges::for_each(_slots, [](auto& slot) { slot.clear(); });

   std::ranges::for_each(
      removed_items, [this](const auto& item) { std::ranges::for_each(_removed_callbacks, [&item](const auto& cb) { cb(item); }); }
   );

   // notify callbacks
   std::ranges::for_each(_updated_callbacks, [](const auto& cb) { cb(); });
}

const std::vector<std::string>& Inventory::getItems() const
{
   return _items;
}

void Inventory::resetKeys()
{
   std::vector<std::string> removed_keys;
   for (const auto& key : _items)
   {
      if (key.starts_with("key"))
      {
         removed_keys.push_back(key);
      }
   }

   _items.erase(std::remove_if(_items.begin(), _items.end(), [](const auto& key) -> bool { return key.starts_with("key"); }), _items.end());

   std::ranges::for_each(
      _slots,
      [](auto& slot)
      {
         if (slot.starts_with("key"))
         {
            slot.clear();
         }
      }
   );

   std::ranges::for_each(
      removed_keys, [this](const auto& item) { std::ranges::for_each(_removed_callbacks, [&item](const auto& cb) { cb(item); }); }
   );

   std::ranges::for_each(_updated_callbacks, [](const auto& cb) { cb(); });
}

std::vector<std::string> Inventory::readItemNames() const
{
   std::vector<std::string> names;
   std::ranges::transform(
      _descriptions, std::back_inserter(names), [](const auto& description) -> std::string { return description._name; }
   );
   return names;
}

void Inventory::selectItem(int32_t slot, const std::string& item)
{
   if (slot < 0 || slot >= static_cast<int32_t>(_slots.size()))
   {
      return;
   }

   _slots[slot] = item;

   // ensure one item is only selected in one slot
   if (!item.empty())
   {
      const auto other_slot_index = 1 - slot;
      if (_slots[other_slot_index] == item)
      {
         _slots[other_slot_index].clear();
      }
   }

   std::ranges::for_each(_updated_callbacks, [](const auto& cb) { cb(); });
}

void Inventory::autoPopulate(const std::string& item)
{
   // add new items to free slots
   const auto& it = std::find_if(_slots.begin(), _slots.end(), [](const auto& slot) { return slot.empty(); });
   if (it != _slots.end())
   {
      *it = item;
   }
}

void Inventory::use(int32_t slot)
{
   const auto& item_name = _slots[slot];
   if (item_name.empty())
   {
      return;
   }

   std::ranges::for_each(
      _used_callbacks,
      [&item_name, this](const auto& cb)
      {
         // call use callback on the item
         if (cb(item_name))
         {
            // if the item has a 'consumed after use' property, remove it
            const auto& it = std::find_if(
               _descriptions.begin(), _descriptions.end(), [item_name](const auto& description) { return description._name == item_name; }
            );

            if (it != _descriptions.end())
            {
               if (it->_properties.find("consumed_after_use") != it->_properties.end())
               {
                  remove(item_name);
               }
            }
         }
      }
   );
}

void Inventory::removeAddedCallback(const AddedCallback& callback_to_remove)
{
   _added_callbacks.erase(
      std::remove_if(
         _added_callbacks.begin(),
         _added_callbacks.end(),
         [&callback_to_remove](const auto& callback)
         {
            const auto match = callback.target_type() == callback_to_remove.target_type() &&
                               callback.template target<AddedCallback>() == callback_to_remove.target<AddedCallback>();

            return match;
         }
      ),
      _added_callbacks.end()
   );
}

void Inventory::removeRemovedCallback(const RemovedCallback& callback_to_remove)
{
   _removed_callbacks.erase(
      std::remove_if(
         _removed_callbacks.begin(),
         _removed_callbacks.end(),
         [&callback_to_remove](const auto& callback)
         {
            const auto match = callback.target_type() == callback_to_remove.target_type() &&
                               callback.template target<RemovedCallback>() == callback_to_remove.target<RemovedCallback>();

            return match;
         }
      ),
      _removed_callbacks.end()
   );
}

void Inventory::removeUsedCallback(const UsedCallback& callback_to_remove)
{
   _used_callbacks.erase(
      std::remove_if(
         _used_callbacks.begin(),
         _used_callbacks.end(),
         [&callback_to_remove](const auto& callback)
         {
            const auto match = callback.target_type() == callback_to_remove.target_type() &&
                               callback.template target<UsedCallback>() == callback_to_remove.target<UsedCallback>();

            return match;
         }

      ),
      _used_callbacks.end()
   );
}

bool Inventory::has(const std::string& item_key) const
{
   const auto it = std::find(_items.cbegin(), _items.cend(), item_key);
   return it != _items.end();
}

void to_json(nlohmann::json& j, const Inventory& d)
{
   j = json{{"items", d._items}, {"slots", d._slots}};
}

void from_json(const nlohmann::json& j, Inventory& d)
{
   if (j.find("items") != j.end())
   {
      d._items = j.at("items").get<std::vector<std::string>>();
   }

   if (j.find("slots") != j.end())
   {
      d._slots = j.at("slots").get<std::array<std::string, 2>>();
   }
}
