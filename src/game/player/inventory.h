#pragma once

#include <functional>

#include "game/player/inventoryitemdescriptionreader.h"

#include "json/json.hpp"

struct Inventory
{
   Inventory();

   using UpdateddCallback = std::function<void()>;
   using UsedCallback = std::function<bool(const std::string&)>;

   void add(const std::string&);
   void remove(const std::string&);
   bool hasInventoryItem(const std::string& item_key) const;
   const std::vector<std::string>& getItems() const;
   void clear();
   void resetKeys();

   void selectItem(int32_t slot, const std::string& item);
   void autoPopulate(const std::string& item);
   void use(int32_t);
   void removeUsedCallback(const UsedCallback& callbackToRemove);

   std::vector<UpdateddCallback> _updated_callbacks;
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
