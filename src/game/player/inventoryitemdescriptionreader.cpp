#include "inventoryitemdescriptionreader.h"

#include "framework/tools/log.h"

#include <filesystem>
#include <fstream>

using json = nlohmann::json;

void InventoryItemDescriptionReader::to_json(nlohmann::json& j, const InventoryItemDescriptionReader::InventoryItemDescription& description)
{
   j = json{
      {"x_px", description._x_px},
      {"y_px", description._y_px},
      {"width_px", description._width_px},
      {"height_px", description._height_px},
      {"title", description._title},
      {"description", description._description},
      {"properties", description._properties}
   };
}

void InventoryItemDescriptionReader::from_json(const json& j, InventoryItemDescriptionReader::InventoryItemDescription& description)
{
   j.at("x_px").get_to(description._x_px);
   j.at("y_px").get_to(description._y_px);
   j.at("width_px").get_to(description._width_px);
   j.at("height_px").get_to(description._height_px);
   j.at("title").get_to(description._title);
   j.at("description").get_to(description._description);

   if (j.find("properties") != j.end())
   {
      j.at("properties").get_to(description._properties);
   }
}

std::vector<InventoryItemDescriptionReader::InventoryItemDescription> InventoryItemDescriptionReader::readItemDescriptions()
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
      std::vector<InventoryItemDescription> images;
      for (auto it = json_data.begin(); it != json_data.end(); ++it)
      {
         InventoryItemDescription image;
         image._name = it.key();
         it.value().get_to(image);
         images.push_back(image);
      }
      return images;
   }
   catch (const std::exception& e)
   {
      Log::Error() << e.what();
   }

   return {};
}
