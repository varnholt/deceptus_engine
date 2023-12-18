#include "inventory.h"

#include <ranges>

using json = nlohmann::json;

void Inventory::add(const std::string& item)
{
   _items.push_back(item);
   std::ranges::for_each(_updated_callbacks, [](const auto& cb) { cb(); });
}

void Inventory::remove(const std::string& item)
{
   auto it = std::remove(_items.begin(), _items.end(), item);
   _items.erase(it, _items.end());
   std::ranges::for_each(_updated_callbacks, [](const auto& cb) { cb(); });
}

void Inventory::clear()
{
   _items.clear();
   std::ranges::for_each(_updated_callbacks, [](const auto& cb) { cb(); });
}

const std::vector<std::string>& Inventory::getItems() const
{
   return _items;
}

void Inventory::resetKeys()
{
   _items.erase(std::remove_if(_items.begin(), _items.end(), [](const auto& key) -> bool { return key.starts_with("key"); }), _items.end());
   std::ranges::for_each(_updated_callbacks, [](const auto& cb) { cb(); });
}

bool Inventory::hasInventoryItem(const std::string& item_key) const
{
   const auto it = std::find(_items.cbegin(), _items.cend(), item_key);
   return it != _items.end();
}

void to_json(nlohmann::json& j, const Inventory& d)
{
   j = json{{"items", d._items}};
}

void from_json(const nlohmann::json& j, Inventory& d)
{
   if (j.find("items") != j.end())
   {
      d._items = j.at("items").get<std::vector<std::string>>();
   }
}
