#include "inventoryimages.h"

#include "framework/tools/log.h"

#include <filesystem>
#include <fstream>

using json = nlohmann::json;

void InventoryImages::to_json(nlohmann::json& j, const InventoryImages::InventoryImage& image)
{
   j = json{
      {"x_px", image._x_px},
      {"y_px", image._y_px},
      {"width_px", image._width_px},
      {"height_px", image._height_px},
   };
}

void InventoryImages::from_json(const json& j, InventoryImages::InventoryImage& image)
{
   image._name = j.begin().key();

   j.at("x_px").get_to(image._x_px);
   j.at("y_px").get_to(image._y_px);
   j.at("width_px").get_to(image._width_px);
   j.at("height_px").get_to(image._height_px);
}

std::vector<InventoryImages::InventoryImage> InventoryImages::readImages()
{
   const auto json_path = "data/sprites/inventory_items.json";

   if (!std::filesystem::exists(json_path))
   {
      return {};
   }

   std::ifstream ifs(json_path, std::ifstream::in);

   auto c = ifs.get();
   std::string data;

   while (ifs.good())
   {
      data.push_back(static_cast<char>(c));
      c = ifs.get();
   }

   ifs.close();

   try
   {
      const json json_data = json::parse(data);
      std::vector<InventoryImage> images = json_data;
      return images;
   }
   catch (const std::exception& e)
   {
      Log::Error() << e.what();
   }

   return {};
}
