#pragma once

#include "json/json.hpp"

#include <functional>

struct Inventory
{
   Inventory() = default;

   void add(const std::string&);
   void remove(const std::string&);
   bool hasInventoryItem(const std::string& item_key) const;
   const std::vector<std::string>& getItems() const;
   void clear();
   void resetKeys();

   void selectItem(int32_t slot, const std::string& item);

   using UpdateddCallback = std::function<void()>;
   std::vector<UpdateddCallback> _updated_callbacks;

   // members
   std::vector<std::string> _items;

   // selected slots
   std::array<std::string, 2> _slots;
};

void to_json(nlohmann::json& j, const Inventory& d);
void from_json(const nlohmann::json& j, Inventory& d);
