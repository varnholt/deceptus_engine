#include "inventoryitem.h"

using json = nlohmann::json;


InventoryItem::InventoryItem(ItemType itemType)
 : _type(itemType)
{
}


void to_json(nlohmann::json& j, const InventoryItem& d)
{
   j = json{
      {"item", static_cast<int32_t>(d._type)}
   };
}


void from_json(const nlohmann::json& j, InventoryItem& d)
{
   d._type = static_cast<ItemType>(j.at("item").get<int32_t>());
}
