#pragma once

#include <functional>

#include "game/player/inventoryitemdescriptionreader.h"

#include "json/json.hpp"

struct Inventory
{
   Inventory();

   using AddedCallback = std::function<void(const std::string&)>;
   using UpdatedCallback = std::function<void()>;
   using UsedCallback = std::function<bool(const std::string&)>;

   void add(const std::string&);
   void remove(const std::string&);
   bool has(const std::string& item_key) const;
   const std::vector<std::string>& getItems() const;
   void clear();
   void resetKeys();
   std::vector<std::string> readItemNames() const;

   void selectItem(int32_t slot, const std::string& item);
   void autoPopulate(const std::string& item);
   void use(int32_t);

   void removeAddedCallback(const AddedCallback& callbackToRemove);
   void removeUsedCallback(const UsedCallback& callbackToRemove);

   std::vector<AddedCallback> _added_callbacks;
   std::vector<UpdatedCallback> _updated_callbacks;
   std::vector<UsedCallback> _used_callbacks;

   // serialized data
   std::vector<std::string> _items;

   // selected slots
   std::array<std::string, 2> _slots;

   // additional inventory data
   std::vector<InventoryItemDescriptionReader::InventoryItemDescription> _descriptions;
};

void to_json(nlohmann::json& j, const Inventory& d);
void from_json(const nlohmann::json& j, Inventory& d);
