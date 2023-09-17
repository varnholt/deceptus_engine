#pragma once

#include "json/json.hpp"

struct Inventory
{
   Inventory() = default;

   void add(const std::string&);
   bool hasInventoryItem(const std::string& item_key) const;
   const std::vector<std::string>& getItems() const;
   void clear();

   void resetKeys();

   std::vector<std::string> _items;
};

void to_json(nlohmann::json& j, const Inventory& d);
void from_json(const nlohmann::json& j, Inventory& d);
