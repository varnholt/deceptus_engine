#pragma once

#include <functional>

#include "game/player/inventoryitemdescriptionreader.h"

#include "json/json.hpp"

/// \brief manages collected inventory item keys, equipped slots, and inventory callbacks.
struct Inventory
{
   /// \brief creates an inventory and loads item descriptions from data files.
   Inventory();

   /// \brief adds an item key, auto-fills a free slot, and notifies updated/added callbacks.
   /// \param item item key to append to the inventory list.
   void add(const std::string&);
   /// \brief removes all matching item keys, clears matching slots, and notifies callbacks.
   /// \param item item key to remove from inventory and slots.
   void remove(const std::string&);
   /// \brief checks whether the inventory currently contains a specific item key.
   /// \param item_key item key to search for.
   /// \return true when item_key exists in _items.
   bool has(const std::string& item_key) const;
   /// \brief returns the serialized item key list.
   /// \return const reference to the internal _items vector.
   const std::vector<std::string>& getItems() const;
   /// \brief removes all items, clears equipped slots, and notifies removal and update callbacks.
   void clear();
   /// \brief removes all key-prefixed items and clears key entries from equipped slots.
   void resetKeys();
   /// \brief returns display names for all known item descriptions.
   /// \return vector containing _name values from loaded item descriptions.
   std::vector<std::string> readItemNames() const;

   /// \brief sets the selected item for a slot and enforces unique selection across both slots.
   /// \param slot inventory slot index.
   /// \param item item key to assign, or empty string to clear the slot.
   void selectItem(int32_t slot, const std::string& item);
   /// \brief places a newly acquired item into the first empty slot.
   /// \param item item key to equip automatically when a free slot exists.
   void autoPopulate(const std::string& item);
   /// \brief triggers use callbacks for the selected slot item and consumes configured items.
   /// \param slot slot index whose selected item should be used.
   void use(int32_t);

   using AddedCallback = std::function<void(const std::string&)>;
   using RemovedCallback = std::function<void(const std::string&)>;
   using UpdatedCallback = std::function<void()>;
   using UsedCallback = std::function<bool(const std::string&)>;

   /// \brief unregisters a previously registered added callback by target identity.
   /// \param callback_to_remove callback instance to remove from _added_callbacks.
   void removeAddedCallback(const AddedCallback& callback_to_remove);
   /// \brief unregisters a previously registered removed callback by target identity.
   /// \param callback_to_remove callback instance to remove from _removed_callbacks.
   void removeRemovedCallback(const RemovedCallback& callback_to_remove);
   /// \brief unregisters a previously registered updated callback by target identity.
   /// \param callback_to_remove callback instance to remove from _updated_callbacks.
   void removeUpdatedCallback(const UpdatedCallback& callback_to_remove);
   /// \brief unregisters a previously registered used callback by target identity.
   /// \param callback_to_remove callback instance to remove from _used_callbacks.
   void removeUsedCallback(const UsedCallback& callback_to_remove);

   std::vector<AddedCallback> _added_callbacks;
   std::vector<RemovedCallback> _removed_callbacks;
   std::vector<UpdatedCallback> _updated_callbacks;
   std::vector<UsedCallback> _used_callbacks;

   // serialized data
   std::vector<std::string> _items;

   // selected slots
   std::array<std::string, 2> _slots;

   // additional inventory data
   std::vector<InventoryItemDescriptionReader::InventoryItemDescription> _descriptions;
};

/// \brief serializes inventory items and slot assignments to json.
/// \param j json object receiving "items" and "slots".
/// \param d inventory source data.
void to_json(nlohmann::json& j, const Inventory& d);
/// \brief deserializes inventory items and slot assignments from json when fields are present.
/// \param j json object that may contain "items" and "slots".
/// \param d inventory target to populate.
void from_json(const nlohmann::json& j, Inventory& d);
