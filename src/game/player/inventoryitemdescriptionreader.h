#ifndef INVENTORYITEMDESCRIPTIONREADER_H
#define INVENTORYITEMDESCRIPTIONREADER_H

#include "json/json.hpp"

namespace InventoryItemDescriptionReader
{

/// \brief describes one inventory item's atlas region, text, and optional properties.
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

/// \brief serializes an inventory item description to json.
/// \param j json object receiving sprite bounds, text fields, and properties.
/// \param d description source data.
void to_json(nlohmann::json& j, const InventoryItemDescription& d);

/// \brief deserializes an inventory item description from json fields.
/// \param j json object with atlas coordinates and text data.
/// \param d description target to populate.
void from_json(const nlohmann::json& j, InventoryItemDescription& d);

/// \brief reads inventory item descriptions from data/sprites/inventory_items.json.
/// \return parsed item descriptions, or an empty vector when the file is missing or invalid.
std::vector<InventoryItemDescription> readItemDescriptions();
}  // namespace InventoryItemDescriptionReader

#endif  // INVENTORYITEMDESCRIPTIONREADER_H
