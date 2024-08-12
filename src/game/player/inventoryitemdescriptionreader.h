#ifndef INVENTORYITEMDESCRIPTIONREADER_H
#define INVENTORYITEMDESCRIPTIONREADER_H

#include "json/json.hpp"

namespace InventoryItemDescriptionReader
{

struct InventoryItemDescription
{
   int32_t _x_px{0};
   int32_t _y_px{0};
   int32_t _width_px{0};
   int32_t _height_px{0};
   std::string _name;
   std::string _title;
   std::string _description;
   std::unordered_map<std::string, std::string> _properties;
};

void to_json(nlohmann::json& j, const InventoryItemDescription& d);
void from_json(const nlohmann::json& j, InventoryItemDescription& d);

std::vector<InventoryItemDescription> readItemDescriptions();
}  // namespace InventoryItemDescriptionReader

#endif  // INVENTORYITEMDESCRIPTIONREADER_H
