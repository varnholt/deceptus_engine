#ifndef INVENTORYIMAGES_H
#define INVENTORYIMAGES_H

#include "json/json.hpp"

namespace InventoryImages
{

struct InventoryImage
{
   int32_t _x_px{0};
   int32_t _y_px{0};
   int32_t _width_px{0};
   int32_t _height_px{0};
   std::string _name;
};

void to_json(nlohmann::json& j, const InventoryImage& d);
void from_json(const nlohmann::json& j, InventoryImage& d);

std::vector<InventoryImage> readImages();
}  // namespace InventoryImages

#endif  // INVENTORYIMAGES_H
