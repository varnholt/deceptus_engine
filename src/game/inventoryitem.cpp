#include "inventoryitem.h"

using json = nlohmann::json;


InventoryItem::InventoryItem(ItemType itemType)
 : mType(itemType)
{
}


void to_json(nlohmann::json& j, const InventoryItem& d)
{
   j = json{
      {"item", static_cast<int32_t>(d.mType)}
   };
}


void from_json(const nlohmann::json& j, InventoryItem& d)
{
   d.mType = static_cast<ItemType>(j.at("item").get<int32_t>());
}
